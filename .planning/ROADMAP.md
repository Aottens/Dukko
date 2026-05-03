# Roadmap: Dukko v1

**Milestone:** M1 — Dukko v1 ("bruikbaar voor productie" in Bitwig on Apple Silicon)
**Created:** 2026-05-03
**Granularity:** standard (5–8 phases)
**Coverage:** 46 / 46 v1 requirements mapped

## Approach

Six phases derived from the research-converged 5-phase build order (STACK + ARCHITECTURE + PITFALLS), with BUILD/CI split out from DSP-scaffold to keep Phase 1 focused and to lock pluginval-strict-10 + LICENSES.md in place before any audio code is written. The spine is dependency-driven: cross-thread plumbing → DSP foundation + state recall → tempo-sync engine on a hardcoded curve → factory-preset UI exercising the snapshot path → editable curve building on proven plumbing → polish + validation.

Audio-thread discipline (no allocations, no locks, no denormals, smoothed parameters, soft-bypass crossfade) is established in Phase 2 — *before* features are layered on. Bitwig state recall is the Phase 2 acceptance gate per research.

## Phases

- [x] **Phase 1: Build foundation & CI** — Pamplejuce-scaffolded VST3 + CLAP build for Apple Silicon, with pluginval-strict-10 green on every push and LICENSES.md tracked from day one.
- [ ] **Phase 2: DSP scaffold & state recall** — Audio-thread-safe passthrough plugin with smoothed depth/wet-dry, soft bypass, and Bitwig save/reopen recalling all parameters intact.
- [ ] **Phase 3: Tempo-sync engine with atomic curve snapshot** — Hardcoded sine-curve duck locked to host PPQ via per-block phase derivation, with the atomic CurveSnapshot publication path proven on the audio thread.
- [ ] **Phase 4: Curve UI & factory presets** — Read-only curve display with live playhead, Kickstart-1 family factory curves baked as static data, click-free preset switching, and resizable UI at 100/125/150%.
- [ ] **Phase 5: Editable curve & user-curve serialization** — User can drag/add/delete points and adjust tension on one user-curve slot, with edits click-free and persisted across Bitwig save/load.
- [ ] **Phase 6: A/B compare, validation & v1 polish** — A/B parameter-snapshot toggle, Steinberg validator + pluginval + Bitwig stress passes, ASan clean, and CPU under target.

## Phase Details

### Phase 1: Build foundation & CI
**Goal**: A Pamplejuce-scaffolded plugin named Dukko builds as native arm64 VST3 + CLAP, loads in Bitwig, and is gated by pluginval-strict-10 in GitHub Actions on every push.
**Depends on**: Nothing (first phase)
**Requirements**: BUILD-01, BUILD-02, BUILD-03, BUILD-04, BUILD-05, QUAL-01, QUAL-05
**Success Criteria** (what must be TRUE):
  1. `cmake --build` produces a Dukko.vst3 and Dukko.clap bundle that load in Bitwig on Apple Silicon, and `lipo -archs` reports `arm64` for both.
  2. The plugin name, manufacturer code, CMake target, plugin manifest, and binary names all read "Dukko" — no "KickstartClone" or "Pamplejuce" leftovers.
  3. Every push to GitHub triggers a `macos-14` Actions job that builds VST3 + CLAP and runs pluginval at strictness level 10 against the VST3 bundle; the badge is green.
  4. `LICENSES.md` exists at repo root and lists JUCE 8, clap-juce-extensions, chowdsp_utils, plus any other dependency pulled in via CPM.
**Plans**: 4 plans
- [x] 01-01-PLAN.md — Pamplejuce ingestion + Dukko rename + bake permanent identifiers (BUILD-01, BUILD-05)
- [x] 01-02-PLAN.md — CPM dep wiring (JUCE 8.0.12, clap-juce-extensions @ SHA, chowdsp_utils v2.4.0) + LICENSES.md (BUILD-03, QUAL-05)
- [x] 01-03-PLAN.md — GitHub Actions workflow: Release+validators job + Debug+ASan job (BUILD-04, QUAL-01)
- [x] 01-04-PLAN.md — Local build + arm64 verification + GitHub repo + first push + manual Bitwig load (BUILD-02)

### Phase 2: DSP scaffold & state recall
**Goal**: An audio-thread-safe passthrough plugin with depth, wet/dry, and click-free soft bypass, where Bitwig save → close → reopen restores every parameter exactly.
**Depends on**: Phase 1
**Requirements**: DSP-02, DSP-03, DSP-04, DSP-05, DSP-06, DSP-07, STAT-01, STAT-02, STAT-03, STAT-04, STAT-05, STAT-06
**Success Criteria** (what must be TRUE):
  1. Toggling bypass and automating depth or wet/dry mid-playback on a sustained signal produces no audible click, pop, or zipper noise (verified on a held pad at 64/128/256/512-sample buffers).
  2. A debug-build allocation guard runs through 10+ minutes of varied use (parameter automation, bypass toggling, project save/load) without ever firing on the audio thread, and `ScopedNoDenormals` is active in `processBlock`.
  3. Saving a Bitwig project with non-default depth, wet/dry, and bypass values, closing Bitwig, and reopening the project restores all three parameters bit-identically; `setState` called twice with the same blob produces identical state.
  4. The serialized state blob is XML headed by `<DukkoState version="1">` containing parameters by stable string ID, and a forward-migration stub exists so future versions can load v1 state.
  5. pluginval at strictness 10 still passes in CI, including its bypass-noise and state-round-trip tests.
**Plans**: TBD

### Phase 3: Tempo-sync engine with atomic curve snapshot
**Goal**: A hardcoded sine-curve duck plays sample-accurately locked to host tempo via PPQ-derived phase, with the atomic CurveSnapshot publication path proven end-to-end.
**Depends on**: Phase 2
**Requirements**: DSP-01, SYNC-01, SYNC-02, SYNC-03, SYNC-04, SYNC-05, CURV-01, CURV-02, CURV-03
**Success Criteria** (what must be TRUE):
  1. Dropping Dukko on a track with a four-on-the-floor kick at 120 BPM and 1/4 sync produces a tight, in-the-pocket pump with no audible drift after 5 minutes of continuous playback in Bitwig.
  2. Looping a region, scrubbing the timeline, and automating tempo from 80 to 160 BPM all leave the duck phase-locked to the bar with no clicks or audible glitches at the discontinuity.
  3. Switching the sync-division parameter through 1/1, 1/2, 1/4, 1/8, 1/16 and their dotted/triplet variants while playing produces no clicks (the LUT crossfade between snapshots absorbs the swap).
  4. With the host stopped or reporting `bpm < 1.0`, the plugin passes audio cleanly with no NaN/Inf in the output and the playhead frozen at its last position.
  5. The audio thread reads only an atomically-published `CurveSnapshot` (no locks, no allocations); a debug log of snapshot pointer addresses confirms the swap happens on parameter change and is consumed cleanly.
**Plans**: TBD

### Phase 4: Curve UI & factory presets
**Goal**: A one-screen UI with a read-only curve display, live playhead, Kickstart-1 family factory presets, and resizable layout makes Dukko visually complete for the factory-preset workflow.
**Depends on**: Phase 3
**Requirements**: CURV-04, CURV-06, CURV-07, UI-01, UI-02, UI-03
**Success Criteria** (what must be TRUE):
  1. Opening the editor shows a single-window UI containing the curve display, depth knob, sync-division dropdown, wet/dry knob, bypass button, preset dropdown, and an A/B button placeholder — all reachable on one screen.
  2. The preset dropdown lists at minimum the Kickstart-1 family (sine, low pulse, high punch, sidechain-style); selecting any of them mid-playback updates the curve display and is audibly click-free.
  3. The curve display renders the active factory curve accurately and a playhead overlay tracks host transport at ~60 Hz with localized repaint (only the playhead column invalidates each frame).
  4. Switching the editor between 100%, 125%, and 150% scale (via a settings menu) re-lays out the entire UI cleanly on a Retina display, with mouse coordinates correctly scaled and no blurry text.
  5. The visual style is recognizably Dukko's own (colors, typography, hierarchy) — readable curves, clear control labelling, not stock-JUCE LookAndFeel and not showroom-grade either.
**Plans**: TBD
**UI hint**: yes

### Phase 5: Editable curve & user-curve serialization
**Goal**: Users can sculpt one custom curve (drag/add/delete points, adjust tension) with no clicks during editing, and the user curve survives Bitwig project save/load with full fidelity.
**Depends on**: Phase 4
**Requirements**: CURV-05, UI-04, UI-05, UI-06, UI-07, UI-09, UI-10
**Success Criteria** (what must be TRUE):
  1. On the user-curve slot, the user can drag any point along its segment, add a new point by double-clicking empty space, delete a point (except endpoints) by right-clicking or double-clicking it, and adjust per-segment tension by alt-dragging — with a ≥4-px click-vs-drag threshold tuned for macOS trackpads.
  2. Editing the curve mid-playback (any of the operations above) produces no clicks; the LUT crossfade established in Phase 3 absorbs every snapshot republication.
  3. Saving a Bitwig project with a hand-edited user curve, closing Bitwig, and reopening the project restores the curve geometry exactly (every point's x, y, and tension within float epsilon).
  4. Every parameter change driven by the curve editor is wrapped in `beginChangeGesture` / `endChangeGesture`, so Bitwig's host-mediated undo can step backward through curve edits and parameter changes alike.
  5. The most common workflow — pick a factory preset from the dropdown, then sweep the depth knob — is reachable in two interactions with no menu round-trips, and the preset dropdown shows a "modified" indicator once the curve diverges from the preset.
**Plans**: TBD
**UI hint**: yes

### Phase 6: A/B compare, validation & v1 polish
**Goal**: Dukko ships v1 with A/B parameter-snapshot compare, passes pluginval + Steinberg + Bitwig stress + ASan clean, and stays under 1% CPU per instance — meeting the "bruikbaar voor productie" bar.
**Depends on**: Phase 5
**Requirements**: DSP-08, UI-08, QUAL-02, QUAL-03, QUAL-04
**Success Criteria** (what must be TRUE):
  1. Pressing the A/B button toggles between two complete plugin-state snapshots (parameters + user curve geometry); copying A→B and editing one side leaves the other untouched, and both snapshots survive Bitwig save/load.
  2. Steinberg's official VST3 validator passes against the Dukko VST3 bundle with zero failures, and pluginval at strictness 10 stays green in CI.
  3. A Bitwig stress session — rapid project switching, sample-rate change from 48 to 96 kHz mid-session, automation-heavy track, 10× project save/reopen cycle — completes with no crashes, no clicks, and no state corruption.
  4. An AddressSanitizer build runs through one full Bitwig session (load, play, edit, save, close) with zero ASan reports.
  5. CPU profiling on M-series at 48 kHz / 256-sample buffer shows Dukko consuming under 1% per instance with the editor open and the playhead overlay running.
**Plans**: TBD
**UI hint**: yes

## Progress

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Build foundation & CI | 0/4 | Planned | — |
| 2. DSP scaffold & state recall | 0/0 | Not started | — |
| 3. Tempo-sync engine with atomic curve snapshot | 0/0 | Not started | — |
| 4. Curve UI & factory presets | 0/0 | Not started | — |
| 5. Editable curve & user-curve serialization | 0/0 | Not started | — |
| 6. A/B compare, validation & v1 polish | 0/0 | Not started | — |

## Coverage Summary

- **Total v1 requirements:** 46
- **Mapped to phases:** 46
- **Orphaned:** 0
- **Duplicated across phases:** 0

| Phase | Req count | Categories represented |
|-------|-----------|------------------------|
| 1 | 7 | BUILD (5), QUAL (2) |
| 2 | 12 | DSP (6), STAT (6) |
| 3 | 9 | DSP (1), SYNC (5), CURV (3) |
| 4 | 6 | CURV (3), UI (3) |
| 5 | 7 | CURV (1), UI (6) |
| 6 | 5 | DSP (1), UI (1), QUAL (3) |

---
*Roadmap created: 2026-05-03*
