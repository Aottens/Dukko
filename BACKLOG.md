# Dukko — Backlog

One sentence per task with a verifiable done criterion; T-shirt size (S ≤ 2 h, M ≤ half day,
L ≤ day, XL = split it); the owner reviews every PR.
Status (first word of the Status cell, checked by `scripts/check-tracking.py` in CI):
`todo` · `doing` · `done` · `gate3` · `blocked`, details in brackets or after a colon.
- A task PR sets its own row to `done (date, #PR)` in the PR itself: **merged = done**.
- `gate3: <checklist path>` = merged, only the owner's run in Bitwig is left; the path must exist.
- Open points found in a run become their own row (or an IDEA) in the same PR.

## Slice 0 — reset

| Id | Task (one sentence, verifiable) | Spec ids | Size | Status |
| --- | --- | --- | --- | --- |
| VS0-01 | The stripped skeleton builds VST3 + CLAP and passes `ctest` locally and in CI (pluginval 10, clap-validator), and the owner sees Dukko load and pass audio in Bitwig. | BUILD-01..06, QUAL-01 | S | done (2026-10-28, #6; run `tests/manual/runs/2026-10-28-VS0-01.md`) |

## Slice 1 — parameters, bypass and state recall (PRD 2)

| Id | Task (one sentence, verifiable) | Spec ids | Size | Status |
| --- | --- | --- | --- | --- |
| VS1-01 | Depth and wet/dry exist as host parameters with permanent string ids, smoothed, and a test shows a step in either produces no jump larger than the smoothing allows. | DSP-02, 03, 05, STAT-01 | M | done (2026-10-28, #7) |
| VS1-02 | Bypass is the host's bypass parameter and crossfades over 5–10 ms; a test shows no sample jump on toggle. | DSP-04 | S | done (2026-10-28, #9; click check by ear in VS2-04) |
| VS1-03 | State is versioned (`<DukkoState version="1">`), load is idempotent and has a migration hook; Bitwig save → close → reopen restores every parameter (checklist). | STAT-02, 05, 06, STAT-04 | M | done (2026-10-29, #12; run `tests/manual/runs/2026-10-29-VS1-03.md`) |
| VS1-05 | Depth and Mix show their value as a whole percentage with a unit (`30 %`) in the host and the generic editor, and typing `30` sets 30 %; a test checks the text in both directions. | DSP-09 | S | done (2026-10-29, #11; run `tests/manual/runs/2026-10-29-VS1-05.md`) |
| VS1-04 | `processBlock` runs under `ScopedNoDenormals`, a debug-build guard fails a test on any allocation in it, and CI has an ASan build that runs the tests. | DSP-06, 07, 10 | M | done (2026-10-29, #13) |

## Slice 2 — tempo sync on a fixed curve (PRD 3)

| Id | Task (one sentence, verifiable) | Spec ids | Size | Status |
| --- | --- | --- | --- | --- |
| VS2-01 | `dsp/` derives the curve phase from PPQ and a sync division (straight, dotted, triplet) per block, guarded against `bpm < 1` and no play head, tested with fake transport. | SYNC-01, 02, 05 | M | done (2026-10-29, #14) |
| VS2-02 | A fixed sine curve ducks the audio by depth at the derived phase; a test checks the gain at known PPQ positions; the processor reads the host play head into `dsp::Transport`. | DSP-01 | S | gate3: `tests/manual/sine-duck.md` |
| VS2-03 | The curve is a point-and-tension model rendered to a 1024-sample LUT and handed to the audio thread by atomic snapshot swap with a crossfade on change. | CURV-01, 02, 03 | L | todo |
| VS2-04 | Owner run in Bitwig: five minutes without drift, loops, jumps and tempo automation without clicks; toggling Dukko's Bypass and Bitwig's device on/off mid-duck gives no click; Division comes back after save and reopen. | SYNC-03, 04, DSP-04, STAT-04 | S | todo |
