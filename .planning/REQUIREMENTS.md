# Requirements: Dukko

**Defined:** 2026-05-03
**Core Value:** Tempo-locked, click-and-go ducking that sounds and feels good in production — drop it on a track, get a tight, musical pumping effect, no fiddling.

> Categories derived from research/FEATURES.md: **Build & Foundation (BUILD)**, **DSP / Audio Engine (DSP)**, **Tempo Sync (SYNC)**, **Curve System (CURV)**, **State & Persistence (STAT)**, **UI / UX (UI)**, **Quality / Validation (QUAL)**.

## v1 Requirements

### Build & Foundation

- [ ] **BUILD-01**: Project scaffolded from Pamplejuce template, building VST3 from CMake on macOS arm64 (Apple Silicon native)
- [ ] **BUILD-02**: Plugin loads in Bitwig (latest stable) on Apple Silicon as native arm64 (verified via `lipo -archs`)
- [ ] **BUILD-03**: Plugin builds and ships as **CLAP** alongside VST3 via `clap-juce-extensions` (Bitwig is CLAP-native)
- [ ] **BUILD-04**: GitHub Actions CI builds VST3 + CLAP on `macos-14` (arm64 runner) on every push
- [ ] **BUILD-05**: Plugin name and identifiers are **Dukko** across CMake target, plugin manifest, and binary names

### DSP / Audio Engine

- [ ] **DSP-01**: Audio processor applies a tempo-synced gain envelope (the duck) to incoming audio with sample-accurate phase
- [ ] **DSP-02**: Depth control (0–100%) determines how deep the duck goes
- [ ] **DSP-03**: Wet/dry mix control (0–100%) blends processed signal with dry — separated from depth (unlike Kickstart's conflated "Mix")
- [ ] **DSP-04**: Click-free soft bypass via VST3 `kIsBypass` flag with crossfade (5–10 ms)
- [ ] **DSP-05**: All audible parameters are smoothed (`SmoothedValue`) so automation is glitch-free
- [ ] **DSP-06**: Audio thread allocates/locks/syscalls *zero* times in steady state — debug-build allocation guard active
- [ ] **DSP-07**: `ScopedNoDenormals` active in `processBlock` — no denormal-induced CPU spikes
- [ ] **DSP-08**: CPU footprint <1% per instance on M-series at 48 kHz / 256-sample buffer

### Tempo Sync

- [ ] **SYNC-01**: Curve syncs to host tempo via `ProcessContext.ppqPosition`, re-derived per block (never accumulated)
- [ ] **SYNC-02**: Sync divisions: 1/1, 1/2, 1/4, 1/8, 1/16, plus dotted (1/2., 1/4., 1/8., 1/16.) and triplet (1/2T, 1/4T, 1/8T, 1/16T) variants — exposed as a single host-automatable parameter
- [ ] **SYNC-03**: Curve is sample-accurate over a 5-minute Bitwig session (zero drift)
- [ ] **SYNC-04**: Loop points, playhead jumps, and tempo automation produce no clicks or phase glitches
- [ ] **SYNC-05**: Defensive `bpm < 1.0` guard prevents NaN/Inf from edge-case host transport state

### Curve System

- [ ] **CURV-01**: Curve internal model: point + tension (Xfer LFOTool-style), serializable, evaluatable in closed form
- [ ] **CURV-02**: Curve is published to audio thread via immutable `CurveSnapshot` (1024-sample LUT) using atomic pointer swap — zero locks
- [ ] **CURV-03**: Crossfade between old and new curve LUT on switch (5–10 ms) — no clicks on preset/curve change
- [ ] **CURV-04**: Factory preset library covers the Kickstart-1 family at minimum: sine, low pulse, high punch, sidechain-style — baked as `static constexpr` data
- [ ] **CURV-05**: Exactly one user-editable curve slot per plugin instance for v1
- [ ] **CURV-06**: Visual curve display reflects the active curve (factory or user) accurately
- [ ] **CURV-07**: Live playhead overlay on curve display follows host transport at ~60 Hz

### State & Persistence

- [ ] **STAT-01**: All plugin parameters use stable string IDs that never change across versions
- [ ] **STAT-02**: Plugin state serializes to versioned XML (`<DukkoState version="1">`) including all parameters + user curve geometry
- [ ] **STAT-03**: Factory curves serialize by index (not embedded), so future factory updates ship transparently
- [ ] **STAT-04**: Bitwig save → close project → reopen project preserves: all parameter values, user curve geometry, A/B snapshots, sync state
- [ ] **STAT-05**: `setState` is deterministic and idempotent (calling twice produces identical state)
- [ ] **STAT-06**: Forward-migration function in place so v1 saved state will load in future versions

### UI / UX

- [ ] **UI-01**: Single-window UI containing: curve display, depth, sync division, wet/dry mix, bypass, preset dropdown, A/B compare
- [ ] **UI-02**: "Strak en bruikbaar" visual style — own visual identity (colors, typography), readable curves, clear hierarchy; not showroom-grade
- [ ] **UI-03**: UI is resizable at **100%, 125%, 150%** zoom — coordinates stored in resolution-independent units, retina-correct
- [ ] **UI-04**: User can drag points on the editable curve to reshape it (click-vs-drag threshold ≥4 px on macOS trackpads)
- [ ] **UI-05**: User can add and delete curve points
- [ ] **UI-06**: User can adjust per-segment tension (curvature between points)
- [ ] **UI-07**: Editing the curve mid-playback produces no clicks (uses LUT crossfade)
- [ ] **UI-08**: A/B compare button toggles between two parameter snapshots (full plugin state, not just curve)
- [ ] **UI-09**: Most common change (preset → depth) is reachable without unnecessary mouse round-trips
- [ ] **UI-10**: Host-mediated undo via `beginGesture`/`endGesture` correctly wraps all parameter changes

### Quality / Validation

- [ ] **QUAL-01**: pluginval at strictness level 10 passes on every CI build (no failures, no warnings beyond known acceptable)
- [ ] **QUAL-02**: Steinberg VST3 SDK validator passes
- [ ] **QUAL-03**: Plugin survives Bitwig stress tests: rapid project switching, sample-rate change mid-session, automation-heavy track
- [ ] **QUAL-04**: AddressSanitizer build runs clean for at least one full Bitwig session
- [ ] **QUAL-05**: `LICENSES.md` tracks JUCE 8, clap-juce-extensions, chowdsp_utils, and any other third-party from day 1

## v2 Requirements

Deferred to v2 (Windows / commercial release / advanced features). Tracked but not in v1 roadmap.

### Windows Port

- **PORT-01**: Plugin builds and runs on Windows 10/11 x64 as VST3 + CLAP
- **PORT-02**: GitHub Actions CI builds Windows artifact alongside macOS
- **PORT-03**: All v1 functional requirements pass on Windows in Bitwig

### Commercial Release Prep

- **REL-01**: Codesigning configured for macOS (Apple Developer ID)
- **REL-02**: Notarization pipeline via `notarytool` integrated in CI
- **REL-03**: Hardened runtime configured per Apple guidelines
- **REL-04**: Installer (`.pkg` for macOS, WiX/Inno for Windows) bundling VST3 + CLAP
- **REL-05**: JUCE 8 EULA re-verified at moment of commercial decision; Steinberg VST3 SDK Usage Agreement signed

### Advanced Features

- **CURV-V2-01**: Multiple curve slots per instance (4–8)
- **CURV-V2-02**: Curve import/export (e.g., as JSON files)
- **CURV-V2-03**: Randomize curve generation
- **SYNC-V2-01**: Tempo offset / push-pull (timing nudge)
- **SYNC-V2-02**: MIDI-trigger mode (start curve on MIDI note)
- **SYNC-V2-03**: Audio-trigger mode (transient detection)
- **SYNC-V2-04**: External sidechain input
- **DSP-V2-01**: Multiband ducking (3 bands + crossovers)
- **DSP-V2-02**: Mid/side processing variants
- **DSP-V2-03**: Oversampling (only if v1 testing reveals aliasing on near-vertical curve edges)
- **DSP-V2-04**: Look-ahead (paired with audio-trigger)
- **UI-V2-01**: Larger zoom levels (200%, 300%) and free-resize handle

## Out of Scope

| Feature | Reason |
|---------|--------|
| AU (Audio Unit) build | Developer uses Bitwig only; AU only matters for Logic Pro users |
| AAX (Pro Tools) build | Requires Avid Developer license + plugin certification — not worth overhead |
| Standalone app build | Plugin lives in DAW; standalone adds no value for the developer |
| Codesigning / notarization / installer (v1) | v1 is local; defer until commercial release decision |
| Multiband ducking (v1) | Category complexity multiplier; not part of "click-and-go" core value |
| MIDI-trigger / audio-trigger / sidechain (v1) | Different sync model than tempo-locked curve; v2 territory |
| Modulation matrix | Bloats scope, dilutes the click-and-go identity (anti-feature per research) |
| MIDI generation/output | Not part of category — anti-feature per research |
| Cloud presets / preset sharing | Out of scope for an own-tool v1 |
| Built-in compressor / EQ / saturation | Anti-features — would push Dukko into VolumeShaper / SoundToys territory and dilute focus |
| AI features | Anti-feature; doesn't serve "click-and-go" core value |
| Free-resize UI handle (drag corner) | Use 100/125/150% presets for v1; free-resize is a v2 polish item |
| In-plugin undo system | Use host-mediated undo (cheaper, sufficient); revisit if Phase 5 testing shows insufficient |

## Traceability

Phase mapping fixed by roadmapper on 2026-05-03. Every v1 requirement maps to exactly one phase.

| Requirement | Phase | Status |
|-------------|-------|--------|
| BUILD-01 | Phase 1 | Pending |
| BUILD-02 | Phase 1 | Pending |
| BUILD-03 | Phase 1 | Pending |
| BUILD-04 | Phase 1 | Pending |
| BUILD-05 | Phase 1 | Pending |
| DSP-01 | Phase 3 | Pending |
| DSP-02 | Phase 2 | Pending |
| DSP-03 | Phase 2 | Pending |
| DSP-04 | Phase 2 | Pending |
| DSP-05 | Phase 2 | Pending |
| DSP-06 | Phase 2 | Pending |
| DSP-07 | Phase 2 | Pending |
| DSP-08 | Phase 6 | Pending |
| SYNC-01 | Phase 3 | Pending |
| SYNC-02 | Phase 3 | Pending |
| SYNC-03 | Phase 3 | Pending |
| SYNC-04 | Phase 3 | Pending |
| SYNC-05 | Phase 3 | Pending |
| CURV-01 | Phase 3 | Pending |
| CURV-02 | Phase 3 | Pending |
| CURV-03 | Phase 3 | Pending |
| CURV-04 | Phase 4 | Pending |
| CURV-05 | Phase 5 | Pending |
| CURV-06 | Phase 4 | Pending |
| CURV-07 | Phase 4 | Pending |
| STAT-01 | Phase 2 | Pending |
| STAT-02 | Phase 2 | Pending |
| STAT-03 | Phase 2 | Pending |
| STAT-04 | Phase 2 | Pending |
| STAT-05 | Phase 2 | Pending |
| STAT-06 | Phase 2 | Pending |
| UI-01 | Phase 4 | Pending |
| UI-02 | Phase 4 | Pending |
| UI-03 | Phase 4 | Pending |
| UI-04 | Phase 5 | Pending |
| UI-05 | Phase 5 | Pending |
| UI-06 | Phase 5 | Pending |
| UI-07 | Phase 5 | Pending |
| UI-08 | Phase 6 | Pending |
| UI-09 | Phase 5 | Pending |
| UI-10 | Phase 5 | Pending |
| QUAL-01 | Phase 1 | Pending |
| QUAL-02 | Phase 6 | Pending |
| QUAL-03 | Phase 6 | Pending |
| QUAL-04 | Phase 6 | Pending |
| QUAL-05 | Phase 1 | Pending |

**Coverage:**
- v1 requirements: 46 total
- Mapped to phases: 46 ✓
- Unmapped: 0
- Duplicated across phases: 0

---
*Requirements defined: 2026-05-03*
*Last updated: 2026-05-03 — traceability filled in by roadmapper (6 phases, 100% coverage)*
