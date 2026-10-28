# Module spec: dsp (DSP)

Status: **DSP-02, 03, 05 approved 2026-10-28 (VS1-01)**; the rest is a draft carried over from the 2026-05-03 requirements, approved when its task starts (gate 1).

## Definitions
- **Curve value** `c` ∈ [0, 1]: how far the duck is in at the current phase (1 = fully ducked). Until VS2-02 the processor feeds `c = 0`.
- **Gain** per sample: `g = 1 − mix × depth × c`, with depth and mix as fractions 0–1. Wet is `x × (1 − depth × c)`, dry is `x`; the output `dry × (1 − mix) + wet × mix` equals `x × g`.

## Acceptance criteria

- **DSP-01** Audio processor applies a tempo-synced gain envelope (the duck) to incoming audio with sample-accurate phase
- **DSP-02** Host parameter `depth`, 0–100 %, default 100 %. At mix 100 % and `c = 1`, depth 100 % gives gain 0 and depth 50 % gives gain 0.5.
- **DSP-03** Host parameter `mix`, 0–100 %, default 100 %, separate from depth. Mix 0 % gives gain 1 for any depth and curve; mix 50 % with depth 100 % and `c = 1` gives gain 0.5.
- **DSP-04** Click-free soft bypass via VST3 `kIsBypass` flag with crossfade (5–10 ms)
- **DSP-05** Depth and mix move linearly to a new value over 20 ms (restarting from the current value on each change). For a step from 0 to 100 % at 48 kHz with `c = 1`, no two consecutive samples differ in gain by more than 1/960, and the gain reaches its target after 960 samples. After `prepare` the values start at their targets, without a ramp.
- **DSP-06** Audio thread allocates/locks/syscalls *zero* times in steady state — debug-build allocation guard active
- **DSP-07** `ScopedNoDenormals` active in `processBlock` — no denormal-induced CPU spikes
- **DSP-08** CPU footprint <1% per instance on M-series at 48 kHz / 256-sample buffer
