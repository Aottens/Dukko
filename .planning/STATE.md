---
gsd_state_version: 1.0
milestone: v1.0
milestone_name: milestone
status: executing
last_updated: "2026-05-03T16:34:01.450Z"
progress:
  total_phases: 6
  completed_phases: 0
  total_plans: 4
  completed_plans: 0
  percent: 0
---

# State: Dukko

**Last updated:** 2026-05-03

## Project Reference

**Project:** Dukko — VST3 + CLAP tempo-synced volume-ducking plugin (Kickstart-class) for Bitwig on Apple Silicon.
**Core value:** Tempo-locked, click-and-go ducking that sounds and feels good in production — drop it on a track, get a tight, musical pumping effect, no fiddling.
**Current milestone:** M1 — Dukko v1 ("bruikbaar voor productie" in Bitwig on Apple Silicon)
**Current focus:** Phase 01 — build-foundation-ci

## Current Position

Phase: 01 (build-foundation-ci) — EXECUTING
Plan: 1 of 4
**Phase:** — (not started)
**Plan:** —
**Status:** Executing Phase 01
**Progress:** Phase 0/6 │░░░░░░░░░░░░░░░░░░░░│ 0%

### Phase Map (M1)

1. Build foundation & CI — *not started*
2. DSP scaffold & state recall — *not started*
3. Tempo-sync engine with atomic curve snapshot — *not started*
4. Curve UI & factory presets — *not started*
5. Editable curve & user-curve serialization — *not started*
6. A/B compare, validation & v1 polish — *not started*

## Performance Metrics

| Metric | Target | Current |
|--------|--------|---------|
| CPU per instance @ 48 kHz / 256 samples | < 1% on M-series | — (measured Phase 6) |
| pluginval strictness 10 in CI | Green every push | — (wired Phase 1) |
| Steinberg VST3 validator | Pass | — (verified Phase 6) |
| Bitwig save → reopen state recall | 100% identical | — (gated Phase 2) |
| Tempo-sync drift over 5 min | 0 audible drift | — (gated Phase 3) |
| Audio-thread allocation count | 0 in steady state | — (gated Phase 2) |

## Accumulated Context

### Decisions

| Date | Decision | Source |
|------|----------|--------|
| 2026-05-03 | Plugin name: **Dukko** (verified unclaimed on KVR + major vendors) | PROJECT.md |
| 2026-05-03 | VST3 + CLAP for v1; AU/AAX excluded; Apple Silicon only for v1 | PROJECT.md |
| 2026-05-03 | Stack: JUCE 8 + CMake/CPM + C++20, scaffolded from Pamplejuce | research/STACK.md |
| 2026-05-03 | CLAP via `clap-juce-extensions` (BUILD-03 in Phase 1, not deferred to polish) | brief constraint |
| 2026-05-03 | Curve representation: point + tension nodes baked into a 1024-sample LUT `CurveSnapshot`, published audio-side via atomic pointer swap | research/ARCHITECTURE.md |
| 2026-05-03 | Phase derived from `ProcessContext.ppqPosition` per block, never accumulated | research/ARCHITECTURE.md |
| 2026-05-03 | State serialized as XML `<DukkoState version="1">`; factory curves by index, user curve geometry inline | research/ARCHITECTURE.md |
| 2026-05-03 | Audio-thread discipline (no allocs, no locks, no denormals, smoothed params, soft bypass) wired in Phase 2 *before* features | research/PITFALLS.md |
| 2026-05-03 | Host-mediated undo for v1 (revisit at Phase 5 start if insufficient) | research/SUMMARY.md |
| 2026-05-03 | Resizable UI at 100/125/150% in Phase 4 (table-stakes; expensive to retrofit) | research/SUMMARY.md |
| 2026-05-03 | A/B compare in Phase 6 — depends on state-snapshot mechanism being solid (Phase 5) | research/SUMMARY.md |
| 2026-05-03 | License re-verification gate: re-read JUCE 8 EULA + sign Steinberg VST3 SDK Usage Agreement at the *moment* of any commercial-release decision (not v1) | research/PITFALLS.md |

### Open Todos

- (none — first plan not yet generated)

### Blockers

- (none)

## Session Continuity

**Next action:** Run `/gsd-plan-phase 1` to generate the plan(s) for Phase 1 (Build foundation & CI).

**Resume hint for new sessions:**

- Read `.planning/PROJECT.md` for product context
- Read `.planning/REQUIREMENTS.md` for v1 scope and traceability
- Read `.planning/ROADMAP.md` for phase structure and success criteria
- Read research files in `.planning/research/` for stack, architecture, and pitfall canon
- Read this STATE.md for current position and accumulated decisions
