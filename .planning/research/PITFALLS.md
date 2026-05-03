# Pitfalls Research

**Domain:** VST3 / CLAP tempo-synced ducking plugin (JUCE-likely framework, Apple Silicon first, possible commercial future)
**Researched:** 2026-05-03
**Confidence:** HIGH for audio-thread / tempo-sync / VST3 traps (well-documented domain knowledge); MEDIUM for Bitwig-specific quirks and current JUCE 8 license terms (verify before commercial release).

> Scope note: every pitfall here is specific to building a tempo-synced volume-ducking VST3 in JUCE-style frameworks on Apple Silicon. Generic "write tests" / "use git" advice is omitted.

---

## Critical Pitfalls

### Pitfall 1: Locks, allocations or syscalls on the audio thread

**What goes wrong:**
The audio callback (`processBlock` in JUCE, `process` in VST3/CLAP) runs on a real-time thread with a hard deadline (e.g. ~5.3 ms at 256 samples / 48 kHz). Any of the following inside it cause dropouts, crackles, or full audio glitches under load:
- `std::mutex::lock()`, `std::shared_mutex`, any blocking primitive
- `new` / `delete` / `malloc` / `std::vector::push_back` that grows / `std::string` that allocates
- File I/O, logging via `std::cout`, `printf`, OSLog
- `dispatch_async`, `NSLog`, anything that crosses into Objective-C runtime
- `std::function` assignment (may allocate), `std::shared_ptr` copy that bumps refcount across threads with atomics-of-doom on weak_ptr

**Why it happens:**
C++ idioms encourage RAII and STL containers. None of those are audio-safe by default. The first time a curve preset is loaded *while playing*, a `std::vector<Point> = newPreset.points;` copy-assigns and frees the old buffer — on the audio thread.

**How to avoid:**
- Treat `processBlock` like an interrupt handler. Maintain a written list of "things that may not appear here": no `new`, no locks, no `String`, no `var`, no `std::function` assignment, no logging.
- Use **lock-free FIFOs** (JUCE `AbstractFifo`, `choc::SingleReaderSingleWriterFIFO`, or `moodycamel::ReaderWriterQueue`) for UI→audio messages.
- Pre-allocate all curve buffers, point arrays, lookup tables in `prepareToPlay` with worst-case sizes. Never resize on the audio thread.
- Use **double-buffered curve state** (current curve + pending curve, atomic pointer swap) for preset switching.
- Read parameters as `std::atomic<float>` or via JUCE `AudioParameterFloat::load()`; never lock to read a param.
- For denormals: enable FTZ/DAZ once per callback (JUCE `ScopedNoDenormals` or `_MM_SET_FLUSH_ZERO_MODE` on x86; on ARM64 use `__builtin_arm_set_fpscr` or rely on the JUCE wrapper which handles ARM).

**Warning signs:**
- Crackles only at low buffer sizes (≤128 samples) but not at 512
- Dropouts coincident with UI actions (preset switch, curve drag, opening editor)
- Profiler shows `__psynch_mutexwait`, `_malloc_zone_*`, or `objc_msgSend` in audio-thread samples
- Pluginval `--strictnessLevel 10` reports allocations during process

**Detection tooling:**
- **Pluginval strict mode** (`--strictnessLevel 10`) — calls process under contrived conditions and watches for state changes outside `setStateInformation`.
- **`farbot::RealtimeObject` / `realtime_check`** or **JUCE `RealtimeAnalyzer`** (debug-only allocation guard via overridden global `operator new` that aborts when the audio thread calls it).
- **Address Sanitizer (ASan)** + **Thread Sanitizer (TSan)** in a debug standalone build — TSan finds non-atomic param reads.
- **Tracy profiler** or **Instruments → System Trace** with the audio thread filtered: any `mach_msg`, `pthread_mutex`, `malloc` events on that thread = bug.

**Phase to address:**
Phase 1 (DSP skeleton): set up `ScopedNoDenormals`, atomic param storage, lock-free FIFO, allocation-guard in debug from day one. Cheap if done early, painful retrofit later.

---

### Pitfall 2: Tempo-sync drift and PPQ-math glitches

**What goes wrong:**
The duck waveform desyncs from the bar over long sessions, jumps audibly when the user loops the timeline, glitches on tempo automation, or starts producing wrong shapes after a time-signature change. Specific failure modes:
- **Float accumulation:** computing `phase += sampleIncrement` per-sample over a 30-minute session accumulates float error → audibly sliding against the grid.
- **Playhead jumps:** user clicks elsewhere on the timeline; plugin's internal phase keeps advancing from the old position → curve no longer aligned to bar 1 of the new region.
- **Loop boundaries:** Bitwig's transport loops cleanly back to the loop start; plugins that integrate phase from `lastPpqPosition + sampleIncrement` instead of recomputing from the host's `ppqPosition` produce a hiccup at the loop point.
- **Tempo automation / tempo ramps:** computing duck length as `samples = (60.0 / bpm) * sampleRate * noteFraction` once per block at block start, but BPM changes mid-block → the duck stretches incorrectly across the block boundary.
- **Time signature changes:** if "1 bar" is hardcoded as 4 beats, a 7/8 section produces a duck of the wrong length.
- **Offline render mismatch:** real-time playback uses `getPlayHead()->getCurrentPosition()` but offline render passes a different, fully-resolved position info. Testing only in real-time misses this.

**Why it happens:**
The "obvious" implementation is to integrate phase from sampleIncrement. The correct implementation is to derive phase from the host's `ppqPosition` every block and only integrate within the block.

**How to avoid:**
- Each `processBlock`, fetch `AudioPlayHead::PositionInfo`. Use **`ppqPosition`** (JUCE 7+) or `juce::Optional<double>` equivalents as ground truth.
- Compute phase as `phase = fmod(ppqPosition * (1.0 / noteFractionInQuarters), 1.0)` at block start; integrate per-sample only within the block using current BPM.
- Re-resolve phase from host on **every** block — never accumulate across blocks.
- Detect playhead jumps: if `|expectedPpq - actualPpq| > tolerance`, treat as a jump; ramp the output through a 5–10 ms crossfade to the new phase to avoid a click.
- Handle `isPlaying == false`: freeze phase at last value, do not advance. When playback resumes, snap to host ppq.
- Handle `ppqPosition == nullopt` (host doesn't provide position info, e.g. in standalone): fall back to a free-running internal clock at the current BPM, but document this.
- For tempo automation, recompute samples-per-cycle every block (cheap); per-sample BPM tracking is overkill for ducking.
- For time signature, derive bar length from `timeSigNumerator / timeSigDenominator`. But for ducking, sync is normally *to a note division*, not to bars — so the time-sig issue mostly matters if you offer "1 bar" as a sync option.

**Warning signs:**
- After playing a 10-minute loop, the duck no longer hits "on the kick"
- Audible click when the user clicks the timeline to a new position
- Bouncing the same project gives a different result than real-time playback
- Tempo automation produces visible curve "stretching" at block boundaries

**Phase to address:**
Phase 2 (sync engine): build the phase computation from host PPQ from day one. Add a "playhead-jump" test (programmatically yank position) to the test plan.

---

### Pitfall 3: Click-causing parameter changes and bypass

**What goes wrong:**
- Direct application of a new gain value produces a step discontinuity → click.
- Bypass toggled mid-buffer flips a multiplier from 0→1 → click.
- Preset switch swaps the curve buffer mid-cycle → discontinuity at the swap sample.
- Wet/dry mix automation produces zipper noise (stair-step) when the host sends parameter changes only at block boundaries.
- "Sample-accurate automation" path in VST3 (`IParameterChanges`) ignored → fast parameter automation sounds chunky.

**Why it happens:**
Audio is hyper-sensitive to discontinuities. Any non-smoothed change of a multiplicative parameter at a non-zero-crossing produces audible click/zipper.

**How to avoid:**
- Wrap every audible parameter (depth, mix, output gain, bypass) in **`juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>`** (or `Multiplicative` for gain in dB). Set ramp time ~10–20 ms.
- For **bypass**: implement *soft bypass* — crossfade between processed and dry over a short ramp (e.g. 5–10 ms). Never hard-toggle. JUCE provides `setBypass`-style hooks; in VST3 use `IBypass` and still ramp internally.
- For **preset / curve switching** during playback: keep the old curve resident, fade out over ~10 ms while fading in the new one (constant-power crossfade). Or gate switches to phase-zero-crossings of the duck cycle.
- For **sample-accurate automation**: process in sub-blocks. JUCE's `AudioProcessor::processBlock` receives `MidiBuffer` and parameter changes via the parameter system; for sub-block accuracy split on parameter change points or call `smoothedValue.getNextValue()` per sample.
- Always start the plugin with smoothers initialized to their target value in `prepareToPlay`, not 0 — otherwise first buffer ramps from silence.
- Test by automating depth from 0→100% over 1 sample and verifying no impulse appears in the output.

**Warning signs:**
- "Click" audible when toggling bypass on a sustained pad
- "Zipper noise" on aggressive depth automation
- Preset dropdown produces a pop when changed during playback

**Phase to address:**
Phase 1 (DSP skeleton): SmoothedValue on every audible param. Phase 3 (preset/curve system): crossfaded curve switching. Phase 4 (bypass + automation polish): IBypass + soft-bypass ramp.

---

### Pitfall 4: DAW state recall bugs (Bitwig)

**What goes wrong:**
- Plugin loads with default values instead of saved state.
- Custom curve geometry is lost on reload (only the preset name survived, not the points).
- A renamed parameter loses automation on reload.
- Saved with v1.0, opened in v1.1 with new params → state read fails silently and resets to defaults.
- Bitwig clones a plugin instance (track duplicate); two instances now share state references inappropriately.
- Total recall passes through `getStateInformation` / `setStateInformation` but the editor doesn't refresh — UI shows stale values.

**Why it happens:**
- Custom curve data isn't a parameter, it's "additional state" that must be serialized in `getStateInformation` separately.
- Param IDs must be **stable strings**, not auto-generated indices. Renaming a param = breaking automation.
- Bitwig is strict about state recall — more so than Live or Logic. Things that "work" in other DAWs may fail here.

**How to avoid:**
- Use **`AudioProcessorValueTreeState`** (JUCE) with explicit string parameter IDs that **never change**. Maintain a parameter ID list as a stable contract.
- Custom curve data → serialize into the same `ValueTree` under a child node (e.g. `<Curve points="..."/>`), recovered in `setStateInformation`.
- **Version every state blob.** Write `<State version="1">` and on load, if `version < current`, run a migration function. Keep old migrations forever.
- **Round-trip test:** start plugin, change all params + curve, save state to XML, recreate plugin, load state, diff parameter values + curve → should be byte-identical (modulo float epsilon).
- After `setStateInformation`, explicitly notify listeners / repaint the editor. Don't assume the editor sees changes via parameter callbacks (the loaded state may bypass the param-attachment path).
- Test in Bitwig specifically: save project, close, reopen, verify state. Bitwig's "preset" file (.bwpreset) and project file (.bwproject) are different paths — test both.
- For curves with N points where N is variable, store N + array. Don't assume fixed-size.
- Store user curve **independently from preset selection**: if the user picks "sine" then drags a point, the dragged version is the truth, not the preset name.

**Warning signs:**
- Reopening a project shows defaults
- Curve looks "close but not quite right" on reload (rounding from a lossy serialization format)
- Automation lanes go to "unmapped" in Bitwig after a plugin update

**Phase to address:**
Phase 1 (skeleton): adopt `AudioProcessorValueTreeState` with stable param IDs. Phase 3 (curve system): add curve serialization with versioning. Phase 5 (validation): add automated round-trip state test + manual Bitwig save/reopen test.

---

### Pitfall 5: VST3 / CLAP gesture and parameter contract violations

**What goes wrong:**
- User drags a knob; host doesn't record automation because the plugin never called `beginGesture` / `endGesture` (`beginChangeGesture` / `endChangeGesture` in JUCE).
- Plugin updates a parameter internally (e.g. tempo-synced LFO modulating depth visually) and erroneously calls `setValueNotifyingHost` for it → host records it as automation, project becomes unwritable.
- Parameter ranges differ from declared ranges → host clipping or scaling mismatch.
- VST3 requires normalized 0..1 range internally; plain values must round-trip through the parameter's `valueToText` / `textToValue`.
- CLAP expects monotonic `param_value` events; sending out-of-order or duplicate events confuses some hosts.
- Sample-rate switch mid-session: host calls `prepareToPlay` again with new SR; if the plugin caches `sampleRate` in static-init or constructor only, all rate-dependent state is wrong.
- Channel layout change (host switches track from mono to stereo): `numChannels` changes; per-channel state arrays must be reallocated *off the audio thread*.

**Why it happens:**
The VST3 / CLAP / JUCE contract is subtle and not enforced at compile time. Many failures are silent — host just records nothing, or records everything.

**How to avoid:**
- Every UI control that changes a parameter must wrap the change in `beginChangeGesture()` … `setValueNotifyingHost(v)` … `endChangeGesture()`. Use JUCE `SliderAttachment` / `ButtonAttachment` / `ComboBoxAttachment` which do this for you.
- Internal-only state (e.g. visual playhead phase) must NOT be a host-visible parameter. Use a separate, non-parameter `Atomic<float>` for UI display.
- Implement and test sample-rate change: in your test harness, instantiate plugin, `prepareToPlay(44100)`, process, `prepareToPlay(96000)`, process → expect no clicks, no crashes, correct sync.
- Test bus layout change: stereo→mono→stereo cycle.
- Use **pluginval** at strictness 10 — it exercises all the above edge cases automatically.
- Denormals: on x86 use `_MM_SET_FLUSH_ZERO_MODE` + `_MM_SET_DENORMALS_ZERO_MODE`; on ARM64 the AArch64 FPCR has a single FZ bit. JUCE `ScopedNoDenormals` abstracts both — use it. Don't write platform-specific intrinsics yourself.

**Warning signs:**
- Bitwig "Automation" view shows no movement when you drag the knob
- Or, opposite: Bitwig records automation when no user interaction happened
- Plugin crashes when host changes sample rate
- Pluginval flags "parameter value out of range" or "gesture not closed"

**Phase to address:**
Phase 1 (skeleton): all parameters via `AudioProcessorValueTreeState`, all UI via Attachment classes. Phase 5 (validation): pluginval strict in CI.

---

### Pitfall 6: Apple Silicon binary / signing pitfalls

**What goes wrong:**
- Plugin built x86_64, runs under Rosetta in Bitwig → works but high CPU and crashes on ARM-only frameworks.
- Plugin built arm64 only → links a third-party static library that's x86_64 only → link-time silent fall-back to fat binary that's actually only x86_64.
- Plugin built fine, but Gatekeeper quarantines it (downloaded build artifact, e.g. from CI) → Bitwig refuses to load with "damaged or incomplete" message even on the dev's own machine.
- Self-built local plugin works; the *same* plugin shared with a tester via AirDrop/zip gets the quarantine bit and fails.
- Universal Binary 2 (`x86_64;arm64`) built but `lipo -info` shows only one slice — silent build misconfig.
- VST3 bundle has wrong Info.plist or missing `Contents/MacOS/<name>` symlink → Bitwig's plugin scanner skips it without telling you why.
- AU validation fails on Apple Silicon Logic but Bitwig loads the VST3 fine — this isn't your problem for v1 but masks bundle issues.

**Why it happens:**
- macOS bundle layout for VST3 is precise: `Plugin.vst3/Contents/{Info.plist,MacOS/Plugin,Resources/}`. CMake / JUCE Projucer normally get this right; manual builds usually don't.
- Codesigning is required for *downloaded* code on macOS. "Self-built locally" bypasses Gatekeeper because the quarantine bit is never set. As soon as the binary travels (CI artifact, airdrop, zip download) it gets quarantined.
- arm64 simulators / dependencies: Homebrew under `/opt/homebrew` is arm64-native; under `/usr/local` is x86_64. Linking against `/usr/local/lib/...` silently x86s your binary.

**How to avoid:**
- Build **arm64 native** for v1 (developer machine). Add the cross-arch (`x86_64;arm64` Universal) build only when commercial release is on the table.
- In CMake: `set(CMAKE_OSX_ARCHITECTURES "arm64")` for v1; later `"arm64;x86_64"`.
- After every build, run `lipo -archs Plugin.vst3/Contents/MacOS/Plugin` and verify it shows `arm64` (or `arm64 x86_64` for universal).
- For dependencies: insist on arm64 slices. Brew packages under `/opt/homebrew` are fine. Avoid `/usr/local`-installed libs. Statically link where possible.
- For Gatekeeper on the dev's own machine: as long as you *build* on the machine, no quarantine. If you ever copy the build elsewhere and back: `xattr -dr com.apple.quarantine /Library/Audio/Plug-Ins/VST3/YourPlugin.vst3`.
- For commercial release: enroll Apple Developer Program ($99/yr), sign with Developer ID Application cert, **notarize** via `notarytool`, **staple** the ticket. Steps: codesign with `--timestamp --options=runtime`, submit to notary service, wait, `xcrun stapler staple`. This applies to the .vst3 bundle, .pkg installer, and any embedded helper tools.
- For v1 (own-use only): explicitly *defer* signing. Document it in a build README so future-you doesn't waste time on it now.

**Warning signs:**
- Activity Monitor shows the plugin's host process under "Intel" architecture
- Bitwig plugin scanner silently skips the plugin (check `~/Library/Application Support/Bitwig/...` logs)
- "is damaged and can't be opened" alert when loading

**Phase to address:**
Phase 0 (project setup / build pipeline): CMake config with `arm64` arch + `lipo` post-build verification. Phase Win-port (later milestone): adjust for cross-platform CI. Commercial-release milestone: codesigning + notarization.

---

### Pitfall 7: Curve editor UX & precision bugs

**What goes wrong:**
- User clicks to add a point but a tiny mouse jitter triggers "drag point" instead → no point added.
- Snap-to-grid feels sticky in some places, loose in others → grid math depends on widget pixel size, not musical units.
- Undo of a curve edit doesn't survive a parameter automation event (parameter change clobbers undo state).
- Curve resolution is 1024 points at the UI level but 64 at DSP level → user sees smooth curve, hears stepped one.
- Curve precision varies with sample rate (curve sampled at SR → at 96 kHz the points fall on different output samples than at 48 kHz, producing tonal differences in bouncing vs playback).
- Drag past the canvas edge → point coordinates go negative or > 1 → DSP indexing crashes or wraps unexpectedly.
- "Clear curve" / "reset" with no undo → user accidentally deletes 5 minutes of work.
- Two-finger trackpad drag ≠ click-drag on some Bitwig configs.

**Why it happens:**
- Curves live in two coordinate systems (pixels for UI, normalized 0..1 musical for DSP); transforms between them are an off-by-one factory.
- Click-vs-drag thresholds are typically 3–5 px on macOS; missing them produces "feels broken" UX.
- Undo systems often track parameter changes only, not "structural" changes like point add/remove.

**How to avoid:**
- Click-vs-drag: require ≥4 px movement before "drag" mode kicks in. Below threshold = treat as click.
- Store curve in **normalized [0..1] x [0..1]** musical coordinates, not pixels. Transform to pixels only at paint time.
- Lookup table: at `prepareToPlay`, render the curve to a fixed-size LUT (e.g. 4096 samples per cycle) regardless of UI resolution. DSP reads the LUT, not the points. SR-independent.
- Snap: separate "visual snap" (cursor snaps to grid for affordance) from "value snap" (held value rounds to grid). Modifier keys should disable both.
- Undo: maintain a per-instance undo stack (cap at e.g. 50 steps). Treat structural changes (add/remove/move-significantly) as undo events; treat in-progress drags as a single undo event committed on mouseUp.
- Bound point coords to [0,1] on input; explicitly clamp on every mutation.
- "Reset curve" → confirmation dialog, or push to undo first.
- For curve interpolation: support at minimum linear + smooth (cubic / Catmull-Rom). Step (sample-and-hold) optional — needed for some EDM-style ducks.

**Warning signs:**
- Beta tester says "I tried to add a point but nothing happens"
- Curve sounds different at 48 vs 96 kHz with identical points
- Project opens with curve looking right but undo stack empty (it should at minimum have "loaded state" as bottom)

**Phase to address:**
Phase 3 (curve system): LUT-based DSP, normalized coords. Phase 6 (UX polish): click-vs-drag, snap, undo.

---

### Pitfall 8: Scope-creep features that look cheap

**What goes wrong:**
Each of these looks like "a few days" but expands to weeks or months. Listed roughly in order of seductiveness for a Kickstart-clone:

| "Just add..." | Real cost |
|---|---|
| **MIDI trigger mode** | Whole second sync path; needs MIDI buffer parsing, retrigger semantics, host-MIDI-routing testing across DAWs, debouncing, note-priority logic. 1–2 weeks. |
| **Multiband** | Crossover filter design (Linkwitz-Riley 4th order min), per-band processing, latency compensation across bands, UI for band edits, presets that scale across bands. 3–6 weeks. |
| **Oversampling** (for the curve edges) | Polyphase FIR design or HIIR, latency reporting (`setLatencySamples`), 2x/4x/8x switching, CPU multiplier, aliasing tests. 1–2 weeks for a correct implementation. Note: ducking by amplitude rarely needs oversampling — only justify if curve generates harmonics that fold. |
| **Lookahead** | Negative latency reporting, sample buffering, host-side compensation, behaves wrong in offline render if not careful. 1 week + a class of new bugs. |
| **Preset browser with tags / search** | UI subsystem, file format, file watching, rename/duplicate/delete, sharing. Indefinite. Stick with a flat dropdown. |
| **A/B compare** | State double-buffering, UI affordance, undo interaction. 2–3 days but invites regressions in state recall. |
| **Audio-rate sidechain input** | Whole second bus, host-routing variability, level matching, defeats "just drop on track" simplicity. Out-of-scope per PROJECT.md — keep it that way. |
| **Per-band depth mod / global LFO** | Adds parameter explosion. |
| **Custom curve shape "morphing"** | Cross-curve interpolation math, UI to choose two curves and a morph param. 1 week. |

**How to avoid:**
- Any feature in this table requires an explicit decision before being added. Default = NO for v1.
- When tempted to add one, ask: "Does it serve the core 'click-and-go ducking that sounds good' value?" If not, defer.
- Re-read `PROJECT.md` Out of Scope before each phase transition.

**Warning signs:**
- Phase plan growing past the original milestone size
- "While I'm here, I might as well..." thoughts

**Phase to address:**
All phases — this is a planning discipline, not a code fix. Roadmap must explicitly enumerate Out of Scope at each milestone boundary.

---

### Pitfall 9: Validator failures (pluginval / Steinberg VST3 validator)

**What goes wrong:**
Plugin loads in Bitwig, sounds fine, but fails Steinberg's official VST3 validator or pluginval. Common reasons:
- **Param count mismatch:** declared param count != actual params returned.
- **Bus layout not supported:** validator probes mono/stereo/5.1; plugin returns true to `isBusesLayoutSupported` but crashes when given that layout.
- **Process called before / after `setActive`:** validator tests the lifecycle.
- **State save not deterministic:** save → load → save produces different bytes.
- **GUI open/close leaks:** open editor 100 times, verify no leaks.
- **Plugin allocates / locks during process:** as in Pitfall 1.
- **Returns wrong unit info:** parameter declared with units that don't match (e.g. "%" range outside 0..100).
- **Doesn't handle 0-length blocks** (some hosts pass empty blocks during transport state changes).
- **Doesn't tolerate `prepareToPlay` being called multiple times back-to-back.**
- **Editor sized incorrectly** (returns negative or zero size before content is laid out).

**How to avoid:**
- Wire **pluginval** into CI from Phase 0:
  ```bash
  pluginval --strictnessLevel 10 --validate-in-process --timeout-ms 30000 path/to/Plugin.vst3
  ```
- Run Steinberg's VST3 validator (`validator` binary in the VST3 SDK) on every build before release.
- Implement `isBusesLayoutSupported` conservatively: return `true` only for mono and stereo. Reject everything else. (You are not building 5.1 ducking.)
- `processBlock` must handle `numSamples == 0` (just return).
- `prepareToPlay` must be idempotent — multiple calls in a row at the same SR/blocksize should be a no-op for state.
- Save → load → save round-trip test in unit tests.

**Warning signs:**
- `pluginval` exits non-zero with `Validation failed:` on any check
- Steinberg validator prints "FAIL" anywhere in its output
- Plugin works in Bitwig but fails in Reaper / Cubase (validator catches what loose hosts forgive)

**Phase to address:**
Phase 0 (CI scaffolding): pluginval in CI on every push. Phase 5 (release-readiness): full Steinberg validator pass.

---

### Pitfall 10: Licensing landmines (JUCE 8, VST3 SDK, CLAP, GPL)

**What goes wrong:**
- Built on JUCE under the free Personal license, then commercial release → must upgrade to Indie ($800) or Pro tier, or risk Splash Screen + GPL3 obligations.
- Used a third-party DSP library that's GPL → entire plugin must be GPL, can't sell as closed-source.
- VST3 SDK is **dual-licensed**: GPLv3 OR proprietary (Steinberg agreement). To ship commercial closed-source you must have signed the **Steinberg VST3 Usage Agreement** (free but requires registration + agreement to terms; renewal expectations).
- CLAP is **MIT** — no obligations beyond attribution. Strictly safer than VST3 for commercial.
- Used a Steinberg-VST3-derived header in your project but didn't sign the agreement → technically infringing.
- Notarization requires an Apple Developer Program membership ($99/yr); without it, downloads of your plugin fail Gatekeeper for users.
- App-specific password / notary credential expires; CI release breaks 6 months in.
- Splash screen / "Made with JUCE" required by Personal-tier license, removed without upgrading → license violation.

**Why it happens:**
The frameworks happily build for personal use without any flag-flipping for commercial obligations. Compliance is on you.

**How to avoid:**
- **JUCE 8** (current as of 2025–2026): Personal tier is free for revenue under a threshold (verify current threshold — was $X per year historically), with mandatory splash screen. Indie removes splash + raises threshold. Pro for higher revenue. **Action: read the latest JUCE EULA at the moment you decide to sell.** Don't assume terms — they have changed before. (LOW confidence on exact 2026 numbers; verify before commercial release.)
- For v1 personal use: any JUCE tier including the free one is fine. Just don't redistribute.
- Maintain a `LICENSES.md` in repo listing every dependency + its license. Update on every dep change.
- Avoid GPL DSP libraries unless you intend to ship GPL. Prefer MIT/BSD/Boost-licensed: e.g. `chowdsp_utils` (BSD), `gin` (check current), DSP code from `eigen` (MPL2 — usable in closed-source).
- Sign the **Steinberg VST3 SDK License Agreement** before any commercial distribution. The 3rdparty/VST3 SDK in JUCE assumes you have. Free, but required.
- Prefer **CLAP first** if you want maximum freedom — it's MIT, no agreement required. Bitwig is CLAP-native, so for the user's primary host, CLAP is actually the friendlier format. VST3 still needed for portability to Cubase / Live / Reaper / FL.
- For Apple notarization: enroll Apple Developer Program before commercial release. Use `notarytool` with an app-specific password stored in `xcrun altool --store-password-in-keychain-item`. Test the full sign+notarize+staple chain before the release deadline, not on it.
- **Don't pull in code from forums or GitHub gists without checking license.** Stack-Overflow code is CC BY-SA 4.0 — incompatible with proprietary closed-source under most lawyers' reading.

**Warning signs:**
- Splash screen visible at runtime in a commercial build (forgot to upgrade JUCE tier)
- A dependency's `LICENSE` file says "GPL"
- VST3 SDK headers in repo without a signed Steinberg agreement on file

**Phase to address:**
Phase 0 (framework choice): pick JUCE tier consistent with eventual commercial intent; track licenses from day one. Commercial-release milestone: re-verify JUCE EULA, sign Steinberg agreement, set up Apple Developer + notarization pipeline.

---

## Technical Debt Patterns

Shortcuts that look reasonable for a personal-use v1 but bite at commercial release.

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|---|---|---|---|
| Hardcode 48 kHz / 256-sample assumptions | Simpler DSP | Breaks at 96 kHz, 192 kHz, 1024-sample buffers; offline render mismatch | NEVER — cost to fix later is high |
| Skip `prepareToPlay` reinit handling | Less code in Phase 1 | Plugin breaks on SR change; pluginval fails | Only as a temp comment-FIXME during prototyping (Phase 1), must fix by Phase 5 |
| Custom param ID format that includes the param's display name | Easy to read | Renaming display name breaks automation | NEVER — IDs must be stable |
| Storing curve as JSON string in a parameter | Avoids "extra state" code | JSON parsing on `setStateInformation` is fine but on audio thread is catastrophic; param strings have length limits in some hosts | NEVER for production |
| Polling host playhead in UI thread for visual playhead | Easy | Tearing, offset between visual and audio | Acceptable in v1 for personal use; replace with audio-thread→UI-thread atomic for commercial polish |
| Skip CLAP entirely | One less format to build | Rework when commercial users want it; missed Bitwig CLAP-native benefits | Acceptable for v1 if VST3 works; revisit at commercial-release decision |
| Single global curve LUT instead of per-instance | Less memory | Two instances with different curves clobber each other | NEVER — must be per-instance |
| Logging via `DBG()` left in `processBlock` | Debugging convenience | DBG allocates a String — audio-thread allocation; OS log buffers fill up | Only if guarded by `#if JUCE_DEBUG` AND not in `processBlock` |
| Skip versioning of state blob | Less code in Phase 1 | First time you change state schema, all old projects break | NEVER — add `version` field from first save |
| Codesigning deferred for v1 | No Apple Developer fee | None for personal use; required for sharing | Acceptable for v1 personal use (matches PROJECT.md) |

---

## Integration Gotchas

| Integration | Common Mistake | Correct Approach |
|---|---|---|
| **Bitwig host** | Assume `getPlayHead()->getPosition()` is always non-null | Always null-check / use `juce::Optional`; fall back to free-running for standalone |
| **Bitwig state recall** | Assume Bitwig saves & restores via the same code path as a fresh project load | Test "save project, close Bitwig, reopen project" specifically — not just "duplicate track" |
| **VST3 IBypass** | Implement bypass purely via a parameter, ignore IBypass interface | Implement IBypass; some hosts (Bitwig is fine, but Cubase, Reaper) only call IBypass, not your bypass param |
| **VST3 Latency** | Forget to call `setLatencySamples` after lookahead config change | Call it in `prepareToPlay` and any time latency changes; host re-aligns automatically |
| **CLAP param events** | Send out-of-order events between blocks | CLAP requires monotonic event time; sort the output param events by `time` field |
| **Apple Audio HAL / CoreAudio** | Assume sample rate is fixed for plugin's lifetime | macOS allows global sample rate change while DAW is open → host calls `prepareToPlay` again |
| **Sandbox / hardened runtime (commercial only)** | Plugin reads from `~/.config/...` outside the bundle | Hardened runtime + notarization restrict file access; use `getAppDataDirectory()` (JUCE) |

---

## Performance Traps

| Trap | Symptoms | Prevention | When It Breaks |
|---|---|---|---|
| Per-sample `std::sin` / curve evaluation via `std::lerp` of cubic spline coefficients | High CPU, ~5%+ per instance | Pre-render curve to a flat LUT in `prepareToPlay`; per-sample is just an indexed lookup | At 8+ instances on a project, or at 96 kHz |
| Allocating temp buffers per-block | Steady-state CPU fine but transient hitches | Pre-allocate scratch buffers in `prepareToPlay`; reuse across calls | Buffer size > 1024 makes per-block alloc cheap; drops are intermittent and hard to repro |
| `juce::Image` repaints on every UI tick | Editor-open CPU 30%+ | Use `setPaintingIsUnclipped(false)` only when correct; cache static parts; repaint dirty rects only; cap visual playhead refresh to ~30 Hz, not 60 | Always; especially on retina + multiple instances |
| Curve interpolation in `processBlock` does branch on point count per sample | Cache misses, CPU spikes on irregular curves | Pre-render to LUT; or use SIMD for parallel interpolation | At ≥4 active points + 96 kHz |
| Smoothed value `getNextValue()` per-channel-per-sample with a non-trivial smoother | Adds up | Compute the smoothed gain once per sample (mono control signal) and apply to all channels | Always negligible for ducking but noticeable in larger plugins |
| Lock-free FIFO with too-small capacity | UI→audio messages dropped under heavy curve editing → state desync | Size FIFO for worst-case burst (e.g. 256 messages); monitor occupancy in debug | When user rapidly drags multiple points |

---

## Security Mistakes

Audio plugins don't have OWASP exposure but still:

| Mistake | Risk | Prevention |
|---|---|---|
| Loading arbitrary preset files via `XmlDocument::parse` without size limit | Malicious preset → crash or memory exhaustion | Cap preset file size (e.g. 1 MB), validate XML structure, sanitize numeric ranges before applying |
| Storing unencoded user paths in state | Path traversal if state is shared between users | Store relative paths or content-hashes, not absolute paths |
| Running outdated VST3 SDK | Known CVE issues fixed in newer versions | Pin to a recent SDK release; subscribe to Steinberg / JUCE security advisories |
| Including telemetry / phone-home in v1 | Privacy concerns; PROJECT.md explicitly says "no telemetry" | Don't add it. If ever added (commercial), opt-in only and document |
| Crashes on malformed `setStateInformation` (corrupted project file) | Crash = data loss in DAW project | Wrap state-load in try/catch; on failure log warning, fall back to defaults, do not crash |

---

## UX Pitfalls

| Pitfall | User Impact | Better Approach |
|---|---|---|
| Curve display doesn't show host playhead position | User can't tell where in the cycle the duck currently is | Animated playhead overlay on the curve, throttled to 30–60 Hz |
| Sync division dropdown buried in a menu | Most-changed control is one extra click | Make sync division and depth the two largest, always-visible controls |
| No visual indication of "depth = 0" being effectively bypass | User wonders why nothing is happening | At depth=0, dim the curve display or show "0% — no ducking" overlay |
| Click on curve background = nothing | User expects to add a point | Click-on-empty-area adds a point at click position; double-click on a point removes it |
| Tooltip-only documentation | New user doesn't discover features | Inline labels for primary controls; tooltips for power-user details |
| Curve preset dropdown changes the curve without confirmation, losing custom edits | User loses work | Mark the dropdown as "modified" when curve diverges from preset; confirm-on-change if modified |
| Bypass LED visually identical to "depth at 0" state | User confused about what's active | Distinct colour / state for true bypass vs zero-depth |
| Decibel vs percentage mismatch on depth | "Depth 50%" — 50% of what? 6 dB? linear amplitude? | Pick one (recommend: linear amplitude reduction, label `depth %` clearly), document. Consider showing `-X dB` next to the % for advanced users |

---

## "Looks Done But Isn't" Checklist

Things that appear complete but are missing critical pieces.

- [ ] **Tempo sync:** Verify works at 30 BPM, 200 BPM, with tempo automation, with time-signature changes, after looping back, after seeking.
- [ ] **Bypass:** Verify it doesn't click on a sustained pad at every buffer size (64, 128, 256, 512, 1024).
- [ ] **Preset switch:** Verify no click during playback at every common buffer size.
- [ ] **State recall:** Save → close Bitwig → reopen → verify ALL params + curve geometry intact.
- [ ] **State recall:** Duplicate track in Bitwig → verify duplicate is independent (not shared state).
- [ ] **Sample rate change:** 44.1 → 96 → 44.1 mid-session → no crash, sync intact, no clicks.
- [ ] **Bus layout:** Mono track → stereo track → no crash; correct ducking on mono.
- [ ] **Editor open/close:** 100 cycles, no leaks (Instruments Allocations).
- [ ] **Validator:** `pluginval --strictnessLevel 10` exits 0.
- [ ] **Validator:** Steinberg VST3 validator passes all checks.
- [ ] **Allocation guard:** debug-build allocation guard never trips during 10 minutes of varied use.
- [ ] **Offline render vs real-time:** bounce a project → A/B identical to real-time playback (within float epsilon).
- [ ] **Automation recording:** drag every knob in Bitwig with automation arm on → verify automation lane records movement.
- [ ] **Universal binary check (when applicable):** `lipo -archs` shows expected slices.
- [ ] **Codesign + notarize (commercial only):** `spctl -a -t exec` accepts the bundle.

---

## Recovery Strategies

| Pitfall | Recovery Cost | Recovery Steps |
|---|---|---|
| Audio-thread allocation discovered late | MEDIUM | Retrofit lock-free FIFO + pre-alloc; hunt allocations with allocation guard; usually 1–3 days if architecture is mostly clean |
| State recall broken after schema change | LOW–MEDIUM | Add migration code that handles old version; ship as patch release; old projects remain readable |
| Tempo-sync drift discovered post-release | MEDIUM | Replace integration with host-PPQ-derived phase; test broadly before patch; users may notice subtle changes |
| Click on bypass | LOW | Add SmoothedValue + crossfade ramp; ship patch |
| Param ID renamed by accident | HIGH | Old projects lose automation. Add legacy alias mapping in `setStateInformation` to translate old IDs to new |
| Plugin universal/arm64 mis-built | LOW | CMake fix + rebuild; users redownload |
| Validator failures discovered late | MEDIUM | Each failure usually a focused fix; budget 1 day per validator FAIL on initial pass |
| GPL contamination via dependency | HIGH | Either go GPL (kills commercial), reimplement the GPL'd code, or find an MIT/BSD equivalent. Audit early to avoid |
| Notarization broken on release day | HIGH | Always test full sign+notarize+staple at least 2 weeks before release; keep a working build artifact as fallback |

---

## Pitfall-to-Phase Mapping

Suggested phase ownership. Phase numbers below are advisory — match them to your actual roadmap structure.

| Pitfall | Prevention Phase | Verification |
|---|---|---|
| Audio-thread safety | Phase 0/1 (skeleton + DSP) | Allocation guard + pluginval strict in CI |
| Tempo-sync drift | Phase 2 (sync engine) | Programmatic playhead-jump + long-session drift test |
| Click on param/bypass/preset | Phase 1 (DSP) + Phase 3 (curve switch) | Step-input click test on sustained tone |
| State recall | Phase 1 (param infra) + Phase 3 (curve) | Round-trip serialization unit test + manual Bitwig project save/reopen |
| VST3/CLAP gestures + lifecycle | Phase 1 (skeleton) + Phase 5 (validation) | pluginval, manual automation-record test in Bitwig |
| Apple Silicon binary | Phase 0 (build pipeline) | `lipo -archs` post-build check |
| Curve UX & precision | Phase 3 (curve system) + Phase 6 (UX polish) | Manual UX test plan + LUT-vs-points sound comparison |
| Scope creep | All phases (planning discipline) | Pre-phase scope review against PROJECT.md Out-of-Scope |
| Validator failures | Phase 0 (CI) + Phase 5 (release prep) | pluginval + Steinberg validator green |
| Licensing | Phase 0 (framework choice) + commercial-release milestone | LICENSES.md audit; signed Steinberg agreement on file before any commercial distribution |

---

## Sources

- **JUCE forums** — long-running threads on `processBlock` allocation safety, `AudioProcessorValueTreeState` patterns, state recall idioms (HIGH confidence — community canonical knowledge).
- **Steinberg VST3 SDK documentation** — gesture, IBypass, lifecycle, validator (HIGH confidence — official).
- **CLAP specification** (github.com/free-audio/clap) — event ordering, MIT license terms (HIGH confidence — official spec).
- **Tracktion `pluginval`** — strictness levels and what each tests (HIGH confidence — open-source tool).
- **Apple Developer documentation** — codesign, notarytool, hardened runtime, universal binaries (HIGH confidence — official).
- **JUCE EULA** — license tiers; **MEDIUM** confidence on 2026-current revenue thresholds; verify at the moment of commercial decision.
- **Bitwig user forums / KVR** — DAW-specific quirks around state recall, parameter automation (MEDIUM confidence — anecdotal but consistent).
- **DSP-stackexchange + KVR DSP forum** — anti-zipper / smoothing patterns, denormal handling (HIGH confidence — established practice).
- Personal-experience-shaped knowledge from the audio plugin developer community: Pirkle, Reiss/McPherson "Audio Effects: Theory, Implementation and Application", Will Pirkle "Designing Audio Effect Plugins in C++" (2nd ed) (HIGH confidence on DSP fundamentals).

---
*Pitfalls research for: VST3 / CLAP tempo-synced ducking plugin (KickstartClone)*
*Researched: 2026-05-03*
