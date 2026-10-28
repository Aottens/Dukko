# Module spec: state (STAT)

Status: **draft, carried over from the 2026-05-03 requirements; the owner approves each criterion when its task starts (gate 1).**

## Acceptance criteria

- **STAT-01** All plugin parameters use stable string IDs that never change across versions
- **STAT-02** Plugin state serializes to versioned XML (`<DukkoState version="1">`) including all parameters + user curve geometry
- **STAT-03** Factory curves serialize by index (not embedded), so future factory updates ship transparently
- **STAT-04** Bitwig save → close project → reopen project preserves: all parameter values, user curve geometry, A/B snapshots, sync state
- **STAT-05** `setState` is deterministic and idempotent (calling twice produces identical state)
- **STAT-06** Forward-migration function in place so v1 saved state will load in future versions
