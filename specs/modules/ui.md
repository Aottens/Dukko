# Module spec: ui (UI)

Status: **draft, carried over from the 2026-05-03 requirements; the owner approves each criterion when its task starts (gate 1).**

## Acceptance criteria

- **UI-01** Single-window UI containing: curve display, depth, sync division, wet/dry mix, bypass, preset dropdown, A/B compare
- **UI-02** "Strak en bruikbaar" visual style — own visual identity (colors, typography), readable curves, clear hierarchy; not showroom-grade
- **UI-03** UI is resizable at **100%, 125%, 150%** zoom — coordinates stored in resolution-independent units, retina-correct
- **UI-04** User can drag points on the editable curve to reshape it (click-vs-drag threshold ≥4 px on macOS trackpads)
- **UI-05** User can add and delete curve points
- **UI-06** User can adjust per-segment tension (curvature between points)
- **UI-07** Editing the curve mid-playback produces no clicks (uses LUT crossfade)
- **UI-08** A/B compare button toggles between two parameter snapshots (full plugin state, not just curve)
- **UI-09** Most common change (preset → depth) is reachable without unnecessary mouse round-trips
- **UI-10** Host-mediated undo via `beginGesture`/`endGesture` correctly wraps all parameter changes
