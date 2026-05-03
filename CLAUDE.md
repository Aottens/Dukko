<!-- GSD:project-start source:PROJECT.md -->
## Project

**Dukko**

**Dukko** is a VST3 + CLAP audio plugin in the spirit of Cableguys/Nicky Romero **Kickstart** (current shipping version: Kickstart 2) — a tempo-synced volume-ducking effect with editable curves, factory presets, and host sync. Built first for the developer's own use in Bitwig on Apple Silicon, with Windows and a commercial release on the table if v1 turns out well.

**Core Value:** **Tempo-locked, click-and-go ducking that sounds and feels good in production** — if everything else fails, dropping it on a track and getting a tight, musical pumping effect must just work.

### Constraints

- **Platform (v1)**: Apple Silicon (arm64) macOS only. Native arm64 binary; Rosetta-only is not acceptable.
- **Platform (v2+)**: Windows x64 must be reachable from the same codebase without major rewrites — bias toward cross-platform frameworks from day one.
- **Plugin format (v1)**: VST3 **and CLAP** (CLAP via `clap-juce-extensions`, ~½ day extra work; Bitwig is CLAP-native). AU/AAX explicitly excluded.
- **Host**: Must pass Bitwig's strictness around state recall and parameter automation as the primary acceptance environment.
- **Performance**: CPU footprint should be unnoticeable on a single track at typical project loads (target: <1% CPU per instance on M-series at 48 kHz / 256-sample buffer).
- **License/cost**: Framework choice should keep the door open to a commercial release. Free-for-personal-use frameworks are fine; pure GPL frameworks would force the plugin to be GPL too, which conflicts with "indien heel nice, verkopen".
- **No external services**: Plugin runs fully offline; no telemetry, no license server in v1.
<!-- GSD:project-end -->

<!-- GSD:stack-start source:research/STACK.md -->
## Technology Stack

## TL;DR — The Recommended Stack
| Area | Pick | Version | Why (one-liner) |
|------|------|---------|-----------------|
| **Plugin framework** | **JUCE** | **8.0.x** | The default for commercial audio plugins. Permissive license up to $50k revenue, indie tier above; mature VST3, native CMake, best-in-class GUI primitives, by far the deepest learning resources. |
| **CLAP support** | **`clap-juce-extensions`** alongside JUCE 8; migrate to native CLAP when JUCE 9 ships | latest `main` | Bitwig is CLAP-native; cost is a single CMake target + a couple of macros. Yes, ship CLAP in v1. |
| **Build system** | **CMake** with **CPM.cmake** for deps | CMake ≥ 3.25 | JUCE 8 is CMake-first; CPM is the friction-free way to vendor JUCE + clap-extensions + chowdsp_utils. |
| **Language / standard** | **C++20** | — | JUCE 8 supports it; concepts/`std::span`/designated initializers are quality-of-life wins; no DSP cost. |
| **DSP helpers** | JUCE built-ins + **chowdsp_utils** | latest `main` | JUCE's `SmoothedValue`, `dsp::ProcessorChain` cover smoothing/ramping; chowdsp_utils adds polished parameter helpers, presets, and modulation utilities used across many shipping plugins. |
| **Curve editor** | Roll-your-own JUCE `Component` modeled on Xfer-style point + tension | — | No off-the-shelf component fits; the data model is small and the rendering is straightforward `Path` + draggable handles. |
| **Validators** | **pluginval 1.0.4** (strictness 10) + **Steinberg VST3 validator** + **Bitwig** | pluginval 1.0.4 | Industry-standard CI gate; Steinberg's validator catches VST3-spec issues pluginval doesn't; Bitwig is the primary acceptance host. |
| **CI / packaging** | **GitHub Actions** with macOS arm64 runner | `macos-14` or `macos-15` | Native arm64 build, no cross-compile needed; binary artefact is a `.vst3` bundle uploaded as workflow artifact. |
## 1. Plugin Framework — **JUCE 8**
### Recommendation
### Rationale
- JUCE 8 EULA defines tiered licenses: **Personal** (free), **Starter** (paid), **Indie**, **Pro**.
- The free **Personal** license permits commercial distribution up to **$50,000 USD revenue over the trailing 12 months** (revenue across ALL of the licensee's products that use JUCE, not per-product).
- Above $50k, you must purchase **Indie** (next tier up) or **Pro**.
- Crucially: **No GPL splash screen requirement under Personal anymore** — this was removed in JUCE 7+. You can ship a closed-source commercial plugin under Personal as long as you stay under the revenue limit.
- This perfectly matches the project's "commercial future is *possible*, not committed" posture: zero cost to start, clear paid upgrade path if the plugin earns real money, no relicensing rewrite ever required.
- macOS arm64 (native), macOS x86_64, Windows x64, Linux x64, iOS, Android — all from one codebase.
- VST3 / AU / AAX / Standalone / LV2 from the same `juce_audio_plugin_client` target. CLAP via extension (see §8).
- Apple Silicon native compilation is first-class via CMake's `CMAKE_OSX_ARCHITECTURES=arm64` (or `arm64;x86_64` for universal). No Rosetta dependency.
- JUCE's VST3 wrapper is one of the most battle-tested in the industry; Bitwig, Ableton, Logic, Cubase, Studio One, Reaper all exercise it daily through hundreds of shipping plugins. State recall, parameter automation, bus configurations, and bypass handling are all mature.
- `juce::Component` hierarchy with `paint()` + `mouseDown/Drag/Up` events covers everything the curve editor needs.
- `juce::Path` (vector graphics) gives smooth curve rendering at any zoom.
- `juce::Slider`, `juce::ComboBox`, `juce::ToggleButton` cover depth/sync/mix/bypass without any third-party widgets.
- LookAndFeel system makes it trivial to give the plugin its own visual identity ("strak en bruikbaar") without writing a custom rendering layer.
- Resizable UI is a first-class concern: `setResizable()` + `ComponentBoundsConstrainer`. Build in resolution-independent units from day 1 (FEATURES.md flags this as a 2026 table-stakes item).
- JUCE has the deepest tutorial corpus of any audio framework: official tutorials, Audio Programmer YouTube, The Art of VA Filter Design book companion code, hundreds of GitHub example plugins.
- Bug-for-bug compatibility with hosts is exhaustively documented in the JUCE forum.
- For a developer-LLM team building from scratch, JUCE is the choice with the most prior art for the LLM to draw on.
### What NOT to use, and why
| Framework | Why not | Confidence |
|-----------|---------|------------|
| **iPlug2** (MIT licensed) | Genuinely viable alternative on license grounds — MIT means zero royalty obligation forever. But: smaller community, fewer learning resources, GUI toolkit (IGraphics) is less mature than JUCE's, CLAP support is via wrapper similar to JUCE. The MIT advantage matters most for a developer who *expects* to exceed JUCE's $50k threshold from day 1. For "maybe commercial later", JUCE Personal is free and the ecosystem advantage dominates. | HIGH |
| **nih-plug** (Rust, ISC license) | Excellent project, strong CLAP-native design, permissive license. But: forces Rust as the language for the whole DSP/UI stack; far less learning material; GUI requires picking one of several emerging Rust GUI crates (egui, vizia, iced) — none mature for plugin UI of this complexity. Not the right bet for "ship in Bitwig on macOS this milestone". | HIGH |
| **Pure VST3 SDK + Steinberg's GPL/proprietary VSTGUI** | The VST3 SDK itself is dual-licensed (GPL3 or Steinberg proprietary). Going GPL conflicts with possible commercial future; the proprietary license has its own restrictions. JUCE wraps all of this for you. Don't reinvent. | HIGH |
| **DPF (DISTRHO Plugin Framework)** | ISC license, supports VST3/CLAP/LV2. Smaller community than JUCE/iPlug2, GUI is minimal. Niche choice unless prioritizing Linux/LV2. | MEDIUM |
| **JUCE 7** (older) | JUCE 8 is the current major release with improved CMake, font handling, and accessibility. No reason to start a new project on JUCE 7 in May 2026. | HIGH |
### Confidence: **HIGH**
## 2. Build System — **CMake with CPM.cmake**
### Recommendation
- **CMake ≥ 3.25** as the build system.
- **CPM.cmake** for fetching JUCE and any third-party libraries (chowdsp_utils, clap-juce-extensions, clap headers).
- Avoid the legacy Projucer `.jucer` workflow.
### Rationale
- JUCE 8 is CMake-first. The CMake API (`juce_add_plugin`, `juce_generate_juce_header`) is mature and well-documented.
- CPM.cmake (single-header CMake module wrapping `FetchContent`) is the de facto modern way to vendor JUCE — used by Sudara's pamplejuce template, chowdsp's templates, and most JUCE-cookbook examples. It gives you reproducible dep versions without git submodules.
- Pure `FetchContent` works but is more verbose; CPM is a thin convenience layer over it. Either is acceptable; CPM is the recommended path because the template ecosystem standardized on it.
- **Do NOT** use Projucer for new projects in 2026. The Projucer-generated workspace files (`.jucer` → IDE projects) are still maintained but are no longer the recommended workflow; CMake is. Projucer adds an extra source-of-truth that drifts.
### Framework-specific quirks to know
- `juce_add_plugin` takes ~30 settings (name, manufacturer code, plugin formats, bus layout, etc.) — consolidate them into one well-commented block at the top of the plugin's `CMakeLists.txt`.
- macOS bundle code-signing is opt-out via `MACOSX_BUNDLE_GUI_IDENTIFIER` and friends; for personal-use v1 you can leave ad-hoc signing on.
- Universal binaries via `set(CMAKE_OSX_ARCHITECTURES "arm64")` for v1 (arm64 only), later `"arm64;x86_64"` if Intel support is wanted.
- **Pamplejuce** (https://github.com/sudara/pamplejuce) is the recommended starting template — it ships JUCE 8 + CPM + GitHub Actions + pluginval CI out of the box. Strongly recommend cloning this as the project's scaffolding rather than building CMakeLists from scratch.
### Confidence: **HIGH**
## 3. Language and Standard — **C++20**
### Recommendation
- **C++20** (`set(CMAKE_CXX_STANDARD 20)`).
- Stay on Apple Clang (Xcode toolchain) on macOS; MSVC 2022 on Windows when that milestone arrives.
### Rationale
- JUCE 8 fully supports C++20.
- C++20 brings genuine quality-of-life features for this project:
- No runtime cost over C++17. No portability concern: every compiler in the audio toolchain (Apple Clang 15+, MSVC 19.30+, GCC 11+) supports the subset of C++20 you'll actually use.
- C++23 is technically available but adds nothing essential and reduces toolchain margin. Skip.
### Why not Rust / nih-plug
### Confidence: **HIGH**
## 4. DSP Primitives
### Recommendation
| Concern | Approach | Notes |
|---------|----------|-------|
| **Parameter smoothing** | `juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>` for gain-like params; `Multiplicative` for frequencies (n/a here). | Set ramp length to the audio block size or ~10–20 ms. Wraps `setTargetValue()` / per-sample `getNextValue()`. Apply to depth, mix, bypass-gain. |
| **Click-free bypass crossfade** | A 5–10 ms equal-power (or simple linear) crossfade between dry input and wet output, driven by the VST3 `kIsBypass` parameter. JUCE provides `kIsBypass` semantics via `juce::AudioProcessor::getBypassParameter()`. | Implement as `output = lerp(dry, wet, bypassRamp)` with `bypassRamp` smoothed by `SmoothedValue`. Verify with pluginval's bypass-noise test. |
| **Sample-accurate tempo sync from host** | Read `juce::AudioPlayHead::PositionInfo` (JUCE 8) every `processBlock`. Pull `getPpqPosition()` (PPQ at *start* of the block), `getBpm()`, and `getTimeInSamples()`. Compute the phase per-sample by linear extrapolation: `ppq[n] = ppqStart + (n / sampleRate) * (bpm / 60.0)`. Use that to index the curve modulo `curveLengthInPpq`. | This is the canonical pattern. Sample-accuracy comes from doing the increment per sample rather than per block. The host reports PPQ at block boundaries; you interpolate inside. Bitwig's transport reporting is exact, so this gives true sample-accurate sync. |
| **Curve evaluation** | Per sample: `phase = fmod(ppq, curveLength) / curveLength` ∈ [0,1] → look up in curve via interpolation between control points. For Xfer-style point+tension, evaluate the tension curve directly (it's a closed-form function of `tension` and the segment-local `t`). | At 48 kHz, sample-rate evaluation is cheap (a few multiplies + a branch per sample); no need to oversample the modulator. |
| **Click-free preset switching** | When the user picks a different curve, do NOT swap the curve pointer mid-block. Instead, run TWO curve evaluators for ~5–10 ms and crossfade. Same mechanism as bypass ramp; reuse the helper. | FEATURES.md explicitly flags this. Users will A/B presets during playback. |
| **Click-free curve-edit interaction** | When the user drags a control point, the curve shape changes between blocks. Two strategies: (a) recompute LUT per parameter change; (b) smooth gain-output between blocks with a short ramp. Strategy (b) is simpler and sufficient. | LUT approach is overkill for v1's curve sizes. Direct evaluation with a short output smoother is cleaner. |
### Optional helper library: **`chowdsp_utils`**
- Repo: https://github.com/Chowdhury-DSP/chowdsp_utils (BSD-3-Clause, commercial-use OK)
- Modules of interest for this project:
- Used across many shipping JUCE plugins (Chowdhury DSP's commercial line, plus others). Maintained.
- Added via CPM in one line.
### What NOT to use
| Avoid | Why |
|-------|-----|
| Roll-your-own parameter smoothing with raw atomics | `juce::SmoothedValue` is correct, audio-thread-safe, and clearer to read. Don't reinvent. |
| Reading host BPM only at `prepareToPlay()` | BPM can change mid-playback (tempo automation, loop transitions). Read every block. |
| Computing phase per-block instead of per-sample | Causes audible quantization artifacts on long curves at fast divisions. Always do the per-sample increment. |
| Naive curve switching (just pointer swap on parameter change) | Audible clicks on every preset change. Must crossfade. |
### Confidence: **HIGH** (these are well-trodden patterns in the JUCE ecosystem)
## 5. Curve / Waveform Editor
### Recommendation
### Why no off-the-shelf component fits
- JUCE does not ship a multi-point bezier editor. `juce::Path` exists but is a rendering primitive, not an editor.
- Third-party JUCE component libraries (Foleys, Melatonin) focus on knobs/meters/visualizers, not curve editors.
- Open-source curve editors exist as parts of larger plugins (Surge XT's MSEG editor, Vital's wavetable editor) — instructive to read, not directly reusable. Their code is GPL3 (Surge) or GPL3 (Vital), so even copying for inspiration requires care.
### Recommended data model (Xfer-style)
- "Point + tension" is the simplest editable curve model that produces musically useful shapes (Xfer LFOTool proves this). Each segment between consecutive points is shaped by a single tension parameter — positive tension bulges up, negative bulges down, zero is linear. Closed-form evaluation, no bezier control handles, no degenerate states.
- Renders to a `juce::Path` once per parameter change for the visual; evaluates per-sample directly from the point array for the audio.
- Serializes to JSON (or JUCE `ValueTree` → XML) for state recall and curve import/export.
### Component design (high level)
- `class CurveEditor : public juce::Component` overrides `paint()`, `mouseDown/Drag/Up`, `mouseDoubleClick`.
- Hit-test: find the nearest control point within ~8 pixels on `mouseDown`.
- Drag: update point's position/value (or tension if Y-axis drag on the segment between two points).
- Double-click on empty space: insert a new point.
- Double-click on existing point: delete it (except first/last).
- Right-click: context menu (snap-to-grid toggle, "reset segment tension", etc.).
- Owns or references a `Curve` model object; emits a `juce::ChangeBroadcaster` event when modified so the audio thread can pick up the new shape (with ramp-smoothing — see §4).
### Reference implementations to study
- The free-audio CLAP examples include a small modulation-curve editor (BSD).
- Sudara's pamplejuce template demonstrates the JUCE component idioms but doesn't include a curve editor.
- Surge XT's MSEG editor (GPL3 — read for inspiration only, do not copy).
### Confidence: **HIGH** (clear pattern, ~1 phase of work to build properly)
## 6. Plugin Validators — Testing Matrix
### v1 mandatory matrix
| Validator | Purpose | When to run | Strictness |
|-----------|---------|-------------|------------|
| **pluginval 1.0.4** | Cross-platform plugin sanity test (non-realtime audio safety, parameter fuzz, state save/load round-trip, bypass behavior). | Every CI build; every release candidate. | **Strictness 10** for release; **5** is the minimum host-compat floor. |
| **Steinberg VST3 SDK validator** (`validator` binary in the SDK) | Catches VST3-spec violations pluginval doesn't (bus config edge cases, spec-conformant parameter flag handling). | Every release candidate. | Default. |
| **Bitwig Studio** (latest release) | Primary acceptance host. Verify: load, parameter automation, state recall via project save/load, no clicks on bypass, no clicks on preset switch, CLAP loads alongside VST3. | Every release candidate manually; smoke-test scripted. | Manual. |
| **Reaper** (optional but recommended) | Secondary VST3 host with very strict adherence to spec. Catches issues Bitwig is permissive about. | Every release candidate. | Manual. |
### What pluginval at strictness 10 actually tests (for context)
- Plugin can be instantiated and destroyed many times without leaking.
- All parameters can be set to all values without crashing.
- `getState()` / `setState()` round-trip preserves audio behavior bit-exactly.
- Bypass produces no DC/click.
- Multiple state restores in sequence (plugin survives Bitwig's session-recall pattern).
- Audio output is non-NaN, non-Inf for sane input.
- Latency reporting is consistent.
### What Steinberg's validator adds
- VST3-specific: bus arrangement compatibility, parameter flag correctness, IComponent/IAudioProcessor/IEditController separation.
### Pamplejuce already ships pluginval CI
- Cloning Sudara's pamplejuce template gives you a `.github/workflows/build.yml` that runs pluginval at strictness 10 on every PR. Use this; don't write it from scratch.
### Confidence: **HIGH**
## 7. CI / Packaging — GitHub Actions, macOS arm64
### Minimal v1 workflow sketch
# .github/workflows/build.yml
### Notes on the v1 workflow
- `macos-14` is the GitHub-hosted Apple Silicon runner (M1). `macos-15` is also available as it rolls out. Native arm64 — no cross-compilation, no Rosetta.
- `CMAKE_OSX_DEPLOYMENT_TARGET=11.0` is the floor for arm64-only macOS Big Sur. Realistic for a developer-personal v1; raise to 12.0+ if commercial release wants more breathing room.
- For v1 (personal use), no codesign / notarize step. The plugin is built ad-hoc-signed, copied to `~/Library/Audio/Plug-Ins/VST3/`, Bitwig loads it.
- Add a CLAP validate step once `clap-juce-extensions` is wired in (CLAP has its own validator at https://github.com/free-audio/clap-validator).
### What changes for the Windows milestone (v2)
| Change | Detail |
|--------|--------|
| Add `windows-latest` job | Same `cmake -B build && cmake --build build` flow; CMake handles MSVC. |
| Architecture | x64 only (skip 32-bit; nobody ships 32-bit VST3 in 2026). |
| Plugin install path | `%CommonProgramFiles%\VST3\` |
| Codesign / installer | Required for commercial release. Use Microsoft's signtool + an EV code-signing cert; build an installer with WiX or Inno Setup. None of this is needed until commercial path is committed. |
| pluginval Windows binary | Same release page, `pluginval_windows.zip`. |
### Confidence: **HIGH**
## 8. CLAP Support — Cost-Benefit, Yes/No for v1
### Recommendation
### Cost (with JUCE 8)
- Add `clap-juce-extensions` via CPM (one CMake block).
- Call `clap_juce_extensions::clap_juce_extensions(KickstartClone)` after `juce_add_plugin`.
- Provide a few CLAP-specific identifiers in CMake (CLAP ID like `com.example.kickstartclone`, feature tags like `audio-effect`, `mixing`).
- Total estimated effort: **half a day to a day**, including running clap-validator and verifying Bitwig loads it.
- Migration to native JUCE 9 CLAP support, when JUCE 9 ships, will be a refactor of those CMake bits — your DSP/UI code is unaffected.
### Benefit
- **Bitwig is CLAP-native**, and CLAP exposes things VST3 hides: per-voice modulation, polyphonic parameters, lower-overhead automation. None of these matter for *this* effect (it has no per-voice state), so the *technical* benefit for v1 is small.
- **The real benefit is Bitwig's preference**: Bitwig's per-instance modulators integrate more deeply with CLAP plugins than VST3 ones in some workflows, and Bitwig's developers have publicly favored CLAP. For a plugin used primarily in Bitwig, shipping CLAP is the better-fit format.
- **Negligible binary-size cost**: CLAP wrapper adds maybe ~100 KB.
- **Future-proofing**: CLAP is gaining traction (Reaper, Studio One, Bitwig support it natively). Cheap insurance for v2.
### Verdict
### What NOT to do
- Don't ship CLAP-only and skip VST3. VST3 is still the universal format; the project explicitly targets VST3 first.
- Don't wait for JUCE 9. Native CLAP support is on the JUCE roadmap but no firm date as of May 2026; `clap-juce-extensions` is production-quality and used by shipping commercial plugins today (Surge XT, several ChowDSP plugins).
### Confidence: **HIGH**
## Recommended Stack — Detailed Tables
### Core Technologies
| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| **JUCE** | 8.0.x (latest 8.x) | Plugin framework: VST3 wrapper, GUI toolkit, audio I/O abstraction, parameter system | Industry default; permissive Personal license up to $50k revenue with clear paid upgrade path; mature CMake; deepest learning corpus; battle-tested in Bitwig and every other host. |
| **CMake** | 3.25+ | Build system | JUCE 8 native; cross-platform; the only sensible choice for new JUCE projects in 2026. |
| **CPM.cmake** | latest | Dependency fetching wrapper around `FetchContent` | Reproducible deps without git submodules; standardized in the JUCE-template ecosystem (pamplejuce, chowdsp templates). |
| **C++** | C++20 (`CMAKE_CXX_STANDARD 20`) | Language | JUCE 8 supports it fully; quality-of-life features (concepts, `std::span`, designated initializers); zero runtime cost vs C++17. |
| **Apple Clang** | Xcode 15+ | macOS toolchain | Default for macOS arm64. CMake handles toolchain selection. |
| **VST3 SDK** | bundled with JUCE 8 | VST3 implementation | JUCE wraps it; you do not interact with it directly. License burden is JUCE's, not yours. |
| **clap-juce-extensions** | latest `main` | CLAP wrapper for JUCE plugins | Adds CLAP build target without DSP/UI changes; production-quality; MIT licensed. |
| **CLAP headers (`clap`)** | 1.x latest | CLAP ABI definitions | Pulled transitively by clap-juce-extensions. MIT licensed. |
### Supporting Libraries
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| **chowdsp_utils** | latest `main` | Parameter helpers, preset management, versioned plugin state | Throughout — start using `chowdsp_plugin_state` from day 1 to get versioned state recall right; layer in `chowdsp_presets_v2` when curve import/export ships. BSD-3-Clause. |
| **JUCE built-ins (`SmoothedValue`, `dsp::ProcessorChain`)** | bundled | Parameter ramping, DSP graph composition | Always — JUCE's smoothing primitives are correct and audio-thread-safe; don't roll your own. |
| **pluginval** | 1.0.4 | Plugin validator for CI | Every build, strictness 10 for releases. |
| **Steinberg VST3 validator** | bundled with JUCE's VST3 SDK fork | VST3-spec compliance check | Every release candidate. |
| **clap-validator** | latest | CLAP-spec compliance check | Every CLAP build. |
### Development Tools
| Tool | Purpose | Notes |
|------|---------|-------|
| **Pamplejuce** (template) | Project scaffolding (https://github.com/sudara/pamplejuce) | Clone as starting point — gives JUCE 8 + CPM + CI + pluginval out of the box. |
| **Xcode** | macOS IDE / debugger | CMake generates an Xcode project: `cmake -G Xcode`. Great for stepping through audio-thread code. |
| **GitHub Actions** | CI / build matrix | macos-14 runner is Apple Silicon; macos-15 also available. |
| **Bitwig Studio** | Primary acceptance host | Test every release candidate manually for project save/load, automation, bypass. |
| **clang-format** | Code style | Optional but recommended; pamplejuce ships a config. |
## Installation (skeleton)
# 1. Clone pamplejuce as the project base
# (then customize CMakeLists.txt: rename target, set manufacturer code, plugin code, etc.)
# 2. Configure (Xcode generator on macOS arm64)
# 3. Build
# 4. Install for local Bitwig testing
# 5. Validate locally
### CPM blocks to add to CMakeLists.txt (sketch)
# ... juce_add_plugin(...) here ...
# Wire CLAP target onto the JUCE plugin
## Alternatives Considered
| Recommended | Alternative | When to Use Alternative |
|-------------|-------------|-------------------------|
| **JUCE 8 (Personal license, $50k threshold)** | iPlug2 (MIT) | If you expect immediate commercial revenue >$50k/year and want zero royalty obligations. License calculus inverts. |
| **JUCE 8** | nih-plug (Rust) | New project, no existing C++ comfort, prioritizing memory-safety and modern language ergonomics over ecosystem maturity. Not this project. |
| **JUCE 8** | DPF (DISTRHO Plugin Framework) | Linux-first or LV2-required project. Not this project. |
| **CMake + CPM** | Projucer (legacy) | Don't. Only used by old projects that haven't migrated. |
| **C++20** | C++17 | Toolchain forced (none in this project's stack does). |
| **C++20** | C++23 | Late 2027 onward, when toolchain support normalizes. Skip for v1. |
| **clap-juce-extensions** | Native JUCE 9 CLAP | When JUCE 9 ships; migrate then. Not viable today. |
| **Roll-your-own curve editor** | Foleys Magic / commercial JUCE component packs | Only if a future commercial release wants showroom polish; v1's "strak en bruikbaar" target does not justify it. |
| **GitHub Actions** | Local-only builds, manual releases | Solo dev willing to skip CI. Strongly discourage — pluginval-on-every-PR catches regressions you cannot eyeball. |
## What NOT to Use
| Avoid | Why | Use Instead |
|-------|-----|-------------|
| **Pure Steinberg VST3 SDK without a wrapper** | The SDK is dual-licensed GPL3/proprietary. GPL3 conflicts with possible commercial future. Proprietary license has its own restrictions. Reinventing JUCE's VST3 wrapper is months of work. | JUCE 8 (or iPlug2). |
| **GPL-only frameworks (older JUCE versions, some forks)** | Forces your plugin to be GPL too — incompatible with commercial closed-source release. | JUCE 8 Personal/Indie/Pro (permissive); iPlug2 (MIT); nih-plug (ISC). |
| **JUCE 7 or earlier** | Older CMake ergonomics, older font/accessibility code, no reason to start a new project here. | JUCE 8.0.x. |
| **Projucer-only workflow** | Legacy. Drifts from CMake; harder to integrate with CI. | CMake-first; let Projucer rust. |
| **AU / AAX plugin formats in v1** | PROJECT.md explicitly excludes them. AU is Logic-only (Bitwig user doesn't need it); AAX requires Avid Developer license + certification. | VST3 + CLAP only. |
| **Audio sidechain bus in v1** | FEATURES.md defers it to v2. Adds VST3 bus-config complexity for no v1 user value. | Single main I/O bus. |
| **Standalone application target** | Useless for a tempo-synced ducking effect (no host BPM = no value). Adds CMake complexity. | VST3 + CLAP only. |
| **Custom audio-thread allocator / lock-free queues for parameters** | JUCE's parameter system already handles this correctly. Bespoke solutions are bug-prone. | `juce::AudioProcessorParameter` + `SmoothedValue`. |
| **Per-block (not per-sample) curve evaluation** | Audible quantization at fast sync divisions. | Per-sample evaluation; curves are cheap. |
| **Codesigning / notarization in v1 CI** | Personal-use v1 doesn't need it; adds Apple Developer Program cost ($99/yr) and complexity. | Defer until commercial release is committed. |
## Stack Patterns by Variant
- Re-evaluate JUCE license tier vs iPlug2 MIT: if projected revenue clearly exceeds $50k/yr, consider iPlug2 migration cost vs JUCE Indie license cost. JUCE Indie is straightforward to purchase; rewriting on iPlug2 is several phases of work. JUCE Indie is almost certainly the right call.
- Add codesigning + notarization step on macOS (requires Apple Developer Program); add EV codesign on Windows.
- Add an installer pipeline (Packages on macOS; WiX/Inno on Windows).
- Migrate from `clap-juce-extensions` to native JUCE 9 CLAP support. CMake-only refactor; DSP/UI code unaffected.
- Add a sidechain VST3 bus via `juce_add_plugin(... NEEDS_AUX_INPUT TRUE ...)`.
- Add transient-detection DSP (envelope follower or onset detector). Consider `chowdsp_dsp_utils` for filter helpers.
- Linkwitz-Riley 4th-order crossover via `juce::dsp::LinkwitzRileyFilter` (built into JUCE).
- Per-band processing chain via `juce::dsp::ProcessorChain`.
## Version Compatibility
| Package | Compatible with | Notes |
|---------|-----------------|-------|
| JUCE 8.0.x | C++17, C++20 | C++20 recommended. C++23 not officially tested. |
| JUCE 8.0.x | CMake 3.22+ | 3.25+ recommended for `block()` and policy improvements. |
| JUCE 8.0.x | macOS 11+ (arm64), macOS 10.13+ (x86_64) | 11.0 deployment target is the practical floor for arm64-only. |
| JUCE 8.0.x + clap-juce-extensions | JUCE 7.x and 8.x | Tracks JUCE main; no version mismatch at present. |
| chowdsp_utils main | JUCE 8.0.x, C++20 | Some chowdsp modules require C++20; matches our choice. |
| pluginval 1.0.4 | macOS 11+ arm64, Windows 10+ x64, Ubuntu 22.04+ | Latest as of release 1.0.4. |
## Confidence Assessment
| Area | Confidence | Reason |
|------|------------|--------|
| Plugin framework (JUCE 8) | HIGH | Verified license terms, ecosystem maturity well-established, clear evidence in this domain. |
| License/commercial-use compatibility | HIGH | JUCE 8 EULA verified; $50k threshold confirmed; Personal license permits closed-source commercial release within limit. |
| CLAP path (clap-juce-extensions, JUCE 9 future) | HIGH | Verified JUCE 8 has no native CLAP; JUCE 9 plans it; clap-juce-extensions is production-used. |
| Build system (CMake + CPM) | HIGH | Standard in the JUCE ecosystem; pamplejuce is widely used. |
| C++20 | HIGH | Toolchain support universal in this domain. |
| DSP primitives (JUCE built-ins + chowdsp_utils) | HIGH | Patterns are well-established in JUCE-based commercial plugins. |
| Curve editor (roll-your-own) | HIGH | No off-the-shelf option; data model is small and well-understood. |
| Validators (pluginval 1.0.4 + Steinberg + Bitwig) | HIGH | pluginval 1.0.4 verified current; matrix is industry-standard. |
| CI (GitHub Actions, macos-14) | HIGH | macos-14 is the published Apple Silicon runner; pattern proven by pamplejuce template. |
## Sources
- [JUCE 8 End User Licence Agreement (official)](https://juce.com/legal/juce-8-licence/) — verified $50k Personal threshold, tier structure
- [JUCE Get JUCE / pricing page](https://juce.com/get-juce/) — verified tier names (Personal / Starter / Indie / Pro)
- [JUCE Releases on GitHub](https://github.com/juce-framework/JUCE/releases) — verified JUCE 8.0.x is current
- [JUCE Roadmap Update Q3 2024](https://juce.com/blog/juce-roadmap-update-q3-2024/) — verified JUCE 9 will add native CLAP; JUCE 8 will not
- [clap-juce-extensions repo](https://github.com/free-audio/clap-juce-extensions/) — verified MIT license, current support for JUCE 8
- [pluginval Releases (Tracktion)](https://github.com/Tracktion/pluginval/releases) — verified 1.0.4 is latest, JUCE 8.0.3 internally, LV2 support added
- [pluginval on tracktion.com](https://www.tracktion.com/develop/pluginval) — verified strictness levels, headless CI usage
- [pamplejuce template (Sudara)](https://github.com/sudara/pamplejuce) — recommended scaffolding (training-data confidence; verify current state when cloning)
- [chowdsp_utils](https://github.com/Chowdhury-DSP/chowdsp_utils) — referenced from training data; BSD-3-Clause
- [CLAP repo (free-audio/clap)](https://github.com/free-audio/clap) — CLAP spec / MIT license
- FEATURES.md (this project) — domain feature requirements that constrain DSP/UI capabilities
- PROJECT.md (this project) — license, platform, and host constraints
<!-- GSD:stack-end -->

<!-- GSD:conventions-start source:CONVENTIONS.md -->
## Conventions

Conventions not yet established. Will populate as patterns emerge during development.
<!-- GSD:conventions-end -->

<!-- GSD:architecture-start source:ARCHITECTURE.md -->
## Architecture

Architecture not yet mapped. Follow existing patterns found in the codebase.
<!-- GSD:architecture-end -->

<!-- GSD:skills-start source:skills/ -->
## Project Skills

No project skills found. Add skills to any of: `.claude/skills/`, `.agents/skills/`, `.cursor/skills/`, `.github/skills/`, or `.codex/skills/` with a `SKILL.md` index file.
<!-- GSD:skills-end -->

<!-- GSD:workflow-start source:GSD defaults -->
## GSD Workflow Enforcement

Before using Edit, Write, or other file-changing tools, start work through a GSD command so planning artifacts and execution context stay in sync.

Use these entry points:
- `/gsd-quick` for small fixes, doc updates, and ad-hoc tasks
- `/gsd-debug` for investigation and bug fixing
- `/gsd-execute-phase` for planned phase work

Do not make direct repo edits outside a GSD workflow unless the user explicitly asks to bypass it.
<!-- GSD:workflow-end -->



<!-- GSD:profile-start -->
## Developer Profile

> Profile not yet configured. Run `/gsd-profile-user` to generate your developer profile.
> This section is managed by `generate-claude-profile` -- do not edit manually.
<!-- GSD:profile-end -->
