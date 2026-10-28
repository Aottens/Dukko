# Module spec: curve (CURV)

Status: **draft, carried over from the 2026-05-03 requirements; the owner approves each criterion when its task starts (gate 1).**

## Acceptance criteria

- **CURV-01** Curve internal model: point + tension (Xfer LFOTool-style), serializable, evaluatable in closed form
- **CURV-02** Curve is published to audio thread via immutable `CurveSnapshot` (1024-sample LUT) using atomic pointer swap — zero locks
- **CURV-03** Crossfade between old and new curve LUT on switch (5–10 ms) — no clicks on preset/curve change
- **CURV-04** Factory preset library covers the Kickstart-1 family at minimum: sine, low pulse, high punch, sidechain-style — baked as `static constexpr` data
- **CURV-05** Exactly one user-editable curve slot per plugin instance for v1
- **CURV-06** Visual curve display reflects the active curve (factory or user) accurately
- **CURV-07** Live playhead overlay on curve display follows host transport at ~60 Hz
