# Module spec: dsp (DSP)

Status: **draft, carried over from the 2026-05-03 requirements; the owner approves each criterion when its task starts (gate 1).**

## Acceptance criteria

- **DSP-01** Audio processor applies a tempo-synced gain envelope (the duck) to incoming audio with sample-accurate phase
- **DSP-02** Depth control (0–100%) determines how deep the duck goes
- **DSP-03** Wet/dry mix control (0–100%) blends processed signal with dry — separated from depth (unlike Kickstart's conflated "Mix")
- **DSP-04** Click-free soft bypass via VST3 `kIsBypass` flag with crossfade (5–10 ms)
- **DSP-05** All audible parameters are smoothed (`SmoothedValue`) so automation is glitch-free
- **DSP-06** Audio thread allocates/locks/syscalls *zero* times in steady state — debug-build allocation guard active
- **DSP-07** `ScopedNoDenormals` active in `processBlock` — no denormal-induced CPU spikes
- **DSP-08** CPU footprint <1% per instance on M-series at 48 kHz / 256-sample buffer
