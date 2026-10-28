# Module spec: state (STAT)

Status: **STAT-01 approved 2026-10-28 (VS1-01)**; the rest is a draft carried over from the 2026-05-03 requirements, approved when its task starts (gate 1).

## Acceptance criteria

- **STAT-01** Every host parameter has a string id and version hint 1 (`juce::ParameterID {id, 1}`); ids so far: `depth`, `mix`. An id never changes or gets reused once released.
- **STAT-02** Plugin state serializes to versioned XML (`<DukkoState version="1">`) including all parameters + user curve geometry
- **STAT-03** Factory curves serialize by index (not embedded), so future factory updates ship transparently
- **STAT-04** Bitwig save → close project → reopen project preserves: all parameter values, user curve geometry, A/B snapshots, sync state
- **STAT-05** `setState` is deterministic and idempotent (calling twice produces identical state)
- **STAT-06** Forward-migration function in place so v1 saved state will load in future versions
