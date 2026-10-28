# Dukko — Architecture

## Data flow

```
 host (Bitwig)                          Dukko
 ─────────────                          ─────
 audio block  ──────────────► PluginProcessor::processBlock ──► audio out
 play head (PPQ, bpm, playing) ──►  phase = f(PPQ, division)      ▲
 parameter changes ──► APVTS atomics ──► smoothed values ─────────┤
                                                                  │
 editor (message thread) ── edits curve ──► new CurveSnapshot ──► atomic swap (lock-free)
```

## Layers
- `source/dsp/` (from the first DSP task): plain C++, no JUCE, no I/O. Curve model and LUT,
  phase from PPQ, gain computation, crossfades. Deterministic; tested with fake PPQ and blocks.
- `source/PluginProcessor.*`: JUCE glue. Parameters (APVTS), state, play-head reading, calls
  into `dsp/`. Nothing here decides how a duck sounds.
- `source/ui/` (from the first UI task): the editor. Reads state, sends parameter changes and
  curve edits; no DSP.

## Threads
- Audio thread: `processBlock` only; the rules in `CLAUDE.md` apply.
- Message thread: editor and state load/save. Talks to the audio thread through parameter
  atomics and an immutable curve snapshot behind an atomic pointer.

## Dependencies
JUCE 8.0.12 (Personal licence), clap-juce-extensions (MIT, pinned SHA), Catch2 3.8.1 (BSL-1.0,
tests only). Tools that run against the binary and are not linked: pluginval (GPLv3),
clap-validator (MIT).
