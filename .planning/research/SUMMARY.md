# Project Research Summary

**Project:** KickstartClone
**Domain:** Tempo-synced volume-ducking VST3/CLAP audio effect (Kickstart-class plugin)
**Researched:** 2026-05-03
**Confidence:** HIGH

## Executive Summary

KickstartClone is a Kickstart-class tempo-synced volume-ducking plugin for Bitwig on Apple Silicon, with VST3 + CLAP as v1 formats and a possible commercial future. The expert path is well-trodden: build on **JUCE 8 + CMake/CPM + C++20**, add CLAP via **`clap-juce-extensions`**, scaffold from the **Pamplejuce** template, and gate every CI build on **pluginval at strictness 10**. The category itself (Cableguys Kickstart 2, VolumeShaper 6, Devious Machines Duck 2, Xfer LFOTool) shows a clear v1 sweet spot: tempo-locked curve playback with editable points, factory presets, click-free preset/bypass switching, and Bitwig-correct state recall — explicitly *not* multiband, audio-trigger, or sidechain.

The architecture is dictated by the audio thread's hard real-time deadline: parameters as `std::atomic` + `SmoothedValue`; curves as a UI-thread-built immutable `CurveSnapshot` (point+tension model baked to a 1024-sample LUT) published to the audio thread by atomic pointer swap; phase derived from host PPQ at every block start (never accumulated); a 5-phase build order (A: plumbing, B: sync engine, C: factory UI, D: editable curve, E: polish) so cross-thread plumbing is proven before features are layered on. Bitwig is the strictest acceptance host for state recall, which makes versioned XML state and a deterministic `setState` non-negotiable from day one.

The dominant risk is audio-thread safety (locks, allocations, syscalls in `processBlock`). Adjacent risks: tempo-sync drift if phase is integrated instead of re-derived from PPQ each block; click-causing parameter/preset/bypass changes without smoothers and crossfades; Bitwig state-recall determinism; and JUCE 8 license re-verification before any commercial release. Every one of these has a known prevention pattern in the JUCE ecosystem, and pluginval-strict-10 in CI catches most of them automatically.

## Key Findings

### Recommended Stack (one paragraph)

**JUCE 8 + CMake/CPM + C++20**, scaffolded from **Pamplejuce**, with **`clap-juce-extensions`** for CLAP alongside VST3, **chowdsp_utils** (BSD-3) for parameter/state polish, validated in CI by **pluginval 1.0.4 (strictness 10)** plus the Steinberg VST3 validator, built on a `macos-14` GitHub Actions runner producing native arm64. JUCE 8's free Personal license covers commercial distribution up to $50k/yr revenue with no splash-screen requirement — zero cost to start with a clear paid upgrade path, perfectly matched to "commercial future *possible*, not committed". CLAP migration to native JUCE 9 support is a CMake-only refactor when JUCE 9 ships.

### v1 Feature Table-Stakes vs Deferred

| Tier | v1 (table stakes + research adds) | Defer (v1.x / v2+) |
|---|---|---|
| **Must have (P1)** | Tempo sync; sync divisions 1/1…1/16 incl. dotted+triplet; depth + wet/dry (separated, unlike Kickstart's conflated "Mix"); click-free bypass (VST3 `kIsBypass` + ramp); click-free preset/curve switching; full parameter automation; Bitwig state recall; Kickstart-1 family factory curves; one editable curve slot; visual curve + playhead; one-screen UI; resizable UI (100/125/150%); host-mediated undo; native arm64 VST3 | — |
| **Should have (P2)** | A/B compare; CLAP build alongside VST3 | Multiple curve slots (4–8); curve import/export; Randomize; tempo offset / push-pull; MIDI-trigger mode |
| **v2+ (P3)** | — | Audio-trigger / transient-following; external sidechain; multiband; look-ahead; swing/groove; mid-side; oversampling; Windows port; codesigning + notarization + installer |
| **Anti-features** | — | Compressor/EQ/saturation; mod matrix; MIDI generation; cloud presets; AI; standalone app |

Strategic free wins over Kickstart 2: (a) wider sync divisions than Kickstart 2 itself ships (1/16, dotted, triplet) at zero DSP cost; (b) A/B compare, conspicuously absent from all four reference products. Naming: brief says "Kickstart 3" but the current shipping product is Kickstart 2 — treat brief's *intent* (current Kickstart as feature ceiling) as binding.

### Architecture (one paragraph)

Single plugin process with two hard-real-time boundaries (audio callback + UI thread). Cross-thread communication is **only** via `std::atomic` for scalars, lock-free FIFO for messages, and an immutable `CurveSnapshot` (point+tension nodes baked to a 1024-sample LUT, with crossfade-on-swap) published by atomic pointer swap. Phase is derived from `ProcessContext.ppqPosition` at the start of every block — never accumulated across blocks — making the plugin sample-accurate and self-correcting on loops, scrubs, and tempo automation. State is versioned XML (`<KickstartCloneState version="1">`) containing parameters + user curve geometry only (factory curves serialize by index so updates ship transparently). One-screen UI in resolution-independent units; playhead overlay polled from atomic at 60 Hz with localized repaint.

### Top 5 Risks / Pitfalls

1. **Audio-thread allocations/locks/syscalls** — *Prevent:* treat `processBlock` like an interrupt handler; pre-allocate in `prepareToPlay`; lock-free FIFO + atomic snapshot pattern; debug-build allocation guard; pluginval strictness 10 in CI from Phase A.
2. **Tempo-sync drift / loop-and-jump glitches** — *Prevent:* re-derive phase from `ppqPosition` every block; integrate per-sample only *within* a block; defensive `bpm < 1.0` check to avoid NaN.
3. **Clicks on bypass / preset switch / parameter automation** — *Prevent:* `SmoothedValue` on every audible parameter; soft-bypass crossfade (5–10 ms); LUT crossfade between old and new `CurveSnapshot` on switch; never hard-toggle.
4. **Bitwig state-recall bugs** — *Prevent:* stable string parameter IDs that never change; serialize curve geometry with versioned blob + forward-migration function; deterministic + idempotent `setState`; explicit editor refresh; manual save→close→reopen test before each milestone.
5. **JUCE 8 license re-verification before commercial release** — *Prevent:* re-read JUCE 8 EULA at the moment of any commercial decision (terms have changed before); sign Steinberg VST3 SDK Usage Agreement before any commercial distribution; maintain `LICENSES.md` from day one. Both STACK.md and PITFALLS.md flag this independently.

## Implications for Roadmap

### Suggested Build Order (5 phases — STACK/ARCHITECTURE/PITFALLS converged independently)

1. **Phase A — Plumbing & DSP scaffold:** Pamplejuce clone loads in Bitwig as native arm64; full parameter set defined; state save/load round-trips; passthrough DSP with smoothed depth + wet/dry + soft bypass. Establishes audio-thread safety and state-recall correctness on a trivial DSP path before features are layered on.
2. **Phase B — Curve engine and tempo sync:** Hardcoded sine-curve duck locked to host tempo via PPQ-derived phase; `TransportSync`, `CurveEngine` LUT lookup, `PlayheadProbe` atomic. Passes a 5-minute-no-drift test in Bitwig.
3. **Phase C — Curve UI and factory presets:** Read-only `CurveEditorComponent` + live playhead overlay; Kickstart-1 family factory curves baked as `static constexpr` data; preset dropdown with click-free LUT crossfade; resizable UI (100/125/150%). Exercises snapshot publication without editor-UX complexity.
4. **Phase D — Editable curve and curve serialization:** Drag/add/delete/tension on the curve component; one user slot serialized into the versioned state blob; click-free during live editing. v1 acceptance test ("bruikbaar voor productie in Bitwig") passes.
5. **Phase E — v1 polish & validation:** A/B compare (reuses state-snapshot mechanism); bypass/loop/jump/tempo-automation/SR-change edge-case audit; CPU profile <1% per instance at 48 kHz/256 samples; Steinberg VST3 validator pass; Bitwig stress test; CLAP build added via `clap-juce-extensions`.

### Phase Ordering Rationale

- Plumbing → DSP → UI: cross-thread architecture is the hardest thing to retrofit; pluginval-strict-10 must stay green at every phase boundary.
- Factory presets *before* editable curves: same `CurveSnapshot` plumbing, but factory swap has zero editor-UX surface area — isolates snapshot/crossfade bugs from drag-handling bugs.
- CLAP in Phase E (not Phase A): `clap-juce-extensions` wraps the JUCE plugin without touching DSP/UI; truly half-day work once VST3 is stable.
- A/B compare in Phase E: depends on state-snapshot mechanism being solid (Phase D); cheap to bolt on after.

### Research Flags

- **Phase A — Bitwig state-recall determinism:** PITFALLS.md notes Bitwig is stricter than other hosts and assigns MEDIUM confidence to Bitwig-specific quirks. Worth a focused validation spike at end of Phase A.
- **Phase D — undo strategy:** PROJECT.md leaves in-plugin vs host-mediated open; default to host-mediated for v1 unless Phase D testing shows it's insufficient.
- **Pre-commercial-release milestone (not a v1 phase):** JUCE 8 EULA re-verification, Steinberg VST3 SDK Usage Agreement signature, Apple Developer Program enrollment, notarization pipeline. Must be a checklist gate before any paid release.

Phases with standard patterns (skip dedicated research): Phase B (canonical PPQ pattern, HIGH confidence); Phase C (vanilla JUCE component work); Phase E (Pamplejuce ships pluginval CI; Steinberg validator is a single binary invocation).

### Conflicts / Open Questions Across Research Files

- **JUCE 8 license re-verification (STACK.md ↔ PITFALLS.md):** both flag this. STACK verified $50k Personal threshold as of May 2026; PITFALLS notes terms have changed historically. **Resolve:** treat current verification as binding for v1; re-verify EULA + sign Steinberg agreement at the *exact moment* of any commercial-release decision.
- **CLAP timing (STACK.md ↔ FEATURES.md ↔ ARCHITECTURE.md):** STACK says v1; FEATURES categorizes P2 (v1.x); ARCHITECTURE treats it as a wrapper concern. **Resolve:** add CLAP in Phase E (still v1) — half-day work, gives Bitwig-native format before first user-facing release.
- **Undo strategy (PROJECT.md ↔ FEATURES.md ↔ ARCHITECTURE.md):** **Resolve:** default host-mediated for v1, revisit at Phase D start.
- **"Kickstart 3" vs "Kickstart 2" naming (PROJECT.md ↔ FEATURES.md):** current shipping product is Kickstart 2 (verified May 2026). **Resolve:** treat brief's intent as binding; update naming reference at next phase transition.
- **Resizable UI scope (FEATURES.md ↔ PROJECT.md):** FEATURES flags as 2026 table stakes; PROJECT.md Active list doesn't currently include it. **Resolve:** add to v1 scope (Phase C); high cost to retrofit.

## Confidence Assessment

| Area | Confidence | Notes |
|---|---|---|
| Stack | HIGH | JUCE 8 EULA + pricing verified; pluginval 1.0.4 confirmed current; Pamplejuce + clap-juce-extensions + chowdsp_utils all production-used. |
| Features | MEDIUM-HIGH | Reference products cross-verified against official sites + reviews; "Kickstart 3" naming reconciled. |
| Architecture | HIGH on patterns, MEDIUM on Bitwig edge cases | Atomic snapshot + per-block PPQ + smoothed parameters are de-facto standards across JUCE/iPlug2/nih-plug. |
| Pitfalls | HIGH on audio/sync/VST3, MEDIUM on Bitwig + license specifics | Audio-thread safety, drift, clicks, validator failures all community-canon. License terms confirmed May 2026; flagged for re-verification. |

**Overall confidence:** HIGH. No exotic territory; every pattern has prior art in shipping plugins.

### Gaps to Address During Planning

- Bitwig CLAP-vs-VST3 acceptance differences — resolve in Phase E by running same stress test against both formats.
- Curve LUT crossfade duration tuning (5–10 ms range) — tune empirically in Phase C.
- In-plugin vs host-mediated undo — flagged for Phase D start.
- Click-vs-drag threshold on macOS trackpads (PITFALLS.md suggests 4 px) — resolve during Phase D UX testing.

## Sources (aggregated from STACK / FEATURES / ARCHITECTURE / PITFALLS)

**Primary (HIGH):** JUCE 8 EULA; JUCE pricing page; JUCE Releases on GitHub; JUCE Roadmap Update Q3 2024; Pamplejuce template (Sudara); clap-juce-extensions; pluginval releases (Tracktion); VST3 SDK documentation (Steinberg); CLAP specification (free-audio); Apple Developer documentation.

**Secondary (MEDIUM):** Cableguys Kickstart official site; Cableguys VolumeShaper 6; Devious Machines Duck 2; Xfer LFOTool; MusicTech / MusicRadar / Sound on Sound product reviews; chowdsp_utils repo; JUCE forum + KVR DSP forum community canon.

**Tertiary (LOW — verify before binding):** Specific Bitwig state-recall edge-case behaviour (re-validate in Phase A); JUCE 8 EULA exact thresholds beyond May 2026 (re-verify before commercial release).
