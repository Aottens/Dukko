# Module spec: sync (SYNC)

Status: **draft, carried over from the 2026-05-03 requirements; the owner approves each criterion when its task starts (gate 1).**

## Acceptance criteria

- **SYNC-01** Curve syncs to host tempo via `ProcessContext.ppqPosition`, re-derived per block (never accumulated)
- **SYNC-02** Sync divisions: 1/1, 1/2, 1/4, 1/8, 1/16, plus dotted (1/2., 1/4., 1/8., 1/16.) and triplet (1/2T, 1/4T, 1/8T, 1/16T) variants — exposed as a single host-automatable parameter
- **SYNC-03** Curve is sample-accurate over a 5-minute Bitwig session (zero drift)
- **SYNC-04** Loop points, playhead jumps, and tempo automation produce no clicks or phase glitches
- **SYNC-05** Defensive `bpm < 1.0` guard prevents NaN/Inf from edge-case host transport state
