# Dukko — Decision Log

Append-only. Newest at top. Each entry: date, decision, alternatives, why, reversal trigger.

---

## 2026-10-28 — Reset onto the CMDMX way of working (owner)

**Decision:** the repo is stripped to a lean JUCE skeleton (one `CMakeLists.txt`, `FetchContent`,
a passthrough plugin, Catch2) and run with the CMDMX loop: spec → test → code → review, tracked
in `BACKLOG.md`/`STATE.md` and checked by `scripts/check-tracking.py`. The GSD planning
(`.planning/`), the Pamplejuce template (`cmake/` helpers, benchmarks, packaging, IPP, Melatonin
inspector, chowdsp_utils) and the old CI are removed. The v1 requirements carry over, ids
intact, into `specs/modules/`.

**Alternatives considered:** keep Pamplejuce and only swap the planning layer.

**Reasoning:** the repo held no product code, only the template; the template brought ~15 CMake
helpers and dependencies nothing used. Starting lean costs one task and leaves less to keep alive.

**Reversal trigger:** a need the template solved returns (installer, IPP): add only that piece.
The old tree stays in git at `749691e`.

## 2026-05-03 — Plugin identity is permanent

**Decision:** product "Dukko", company "Dukko Audio", manufacturer code `Dukk`, plugin code
`Dkk1`, bundle and CLAP id `com.dukkoaudio.dukko`.

**Reasoning:** hosts store these in saved projects; a Dukko build is already installed and
loaded in Bitwig on the owner's Mac.

**Reversal trigger:** none. Changing them breaks every saved project.

## 2026-05-03 — Carried-over design decisions (from the GSD research, still valid)

- VST3 + CLAP for v1 (Bitwig is CLAP-native); AU and AAX out.
- Stack JUCE 8 + CMake + C++20; keeps a commercial release possible.
- Curve model: points with tension, rendered to a 1024-sample LUT, published to the audio
  thread as an immutable snapshot by atomic pointer swap.
- Phase is derived from the host PPQ every block, never accumulated.
- A/B compare and resizable UI (100/125/150 %) are in v1, planned in from the start.
- Undo is the host's, through parameter gestures.
