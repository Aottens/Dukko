# Module spec: dsp (DSP)

Status: **DSP-02, 03, 05 approved 2026-10-28 (VS1-01), DSP-04 approved 2026-10-28 (VS1-02), DSP-09 approved 2026-10-28 (VS1-05), DSP-06, 07, 10 approved 2026-10-29 (VS1-04)**; the rest is a draft carried over from the 2026-05-03 requirements, approved when its task starts (gate 1).

## Definitions
- **Curve value** `c` ∈ [0, 1]: how far the duck is in at the current phase (1 = fully ducked). Until VS2-02 the processor feeds `c = 0`.
- **Gain** per sample: `g = 1 − mix × depth × c`, with depth and mix as fractions 0–1. Wet is `x × (1 − depth × c)`, dry is `x`; the output `dry × (1 − mix) + wet × mix` equals `x × g`.
- **Bypass factor** `b` ∈ [0, 1]: 0 = active, 1 = bypassed. With it the gain is `g = 1 − (1 − b) × mix × depth × c`.

## Acceptance criteria

- **DSP-01** Audio processor applies a tempo-synced gain envelope (the duck) to incoming audio with sample-accurate phase
- **DSP-02** Host parameter `depth`, 0–100 %, default 100 %. At mix 100 % and `c = 1`, depth 100 % gives gain 0 and depth 50 % gives gain 0.5.
- **DSP-03** Host parameter `mix`, 0–100 %, default 100 %, separate from depth. Mix 0 % gives gain 1 for any depth and curve; mix 50 % with depth 100 % and `c = 1` gives gain 0.5.
- **DSP-04** Host parameter `bypass` (on/off, default off) is the plugin's bypass parameter (`getBypassParameter`), so Bitwig's device on/off, VST3 `kIsBypass` and CLAP bypass all drive it. Toggling ramps `b` linearly over 10 ms. At 48 kHz with depth, mix and `c` at 1, no two consecutive samples differ in gain by more than 1/480, and after 480 samples the gain is exactly 1 (bypassed) or 0 (active). After `prepare`, `b` starts at its target. Latency is 0 in both states.
- **DSP-05** Depth and mix move linearly to a new value over 20 ms (restarting from the current value on each change). For a step from 0 to 100 % at 48 kHz with `c = 1`, no two consecutive samples differ in gain by more than 1/960, and the gain reaches its target after 960 samples. After `prepare` the values start at their targets, without a ramp.
- **DSP-09** Depth and mix show as a whole percentage with a space and a unit: the value rounded to the nearest integer (halves round up), so 30.4 reads `30 %`, 99.6 reads `100 %`, 0 reads `0 %`. The parameter has no separate label, so no host shows the unit twice. Typed text sets the value it names: `30`, `30 %` and `30%` set 30 %, `30.4` sets 30.4 % (not rounded; only the display rounds), values outside 0–100 clamp (`150` → 100 %, `-5` → 0 %), and text without a leading number sets the default, 100 %.
- **DSP-06** `processBlock` does not allocate. The test executable replaces every global `operator new` and `operator delete` form (plain, array, nothrow, aligned) with versions that call `malloc`/`free` and, while a guard is armed on the calling thread, count each allocation. A test prepares the plugin at 48 kHz / 256, then with the guard armed only around each `processBlock` call runs 100 stereo blocks of 256, 1 and 37 samples while depth, mix and bypass change between blocks; the count stays 0. A self-test shows the guard counts one `new int` made while armed and none while disarmed or on another thread. The guard lives in `tests/` only, so it runs in every build type and never ships in the plugin. Locks and syscalls are not tested; they stay a review rule (CLAUDE.md, audio-thread rules).
- **DSP-07** `processBlock` runs with denormals flushed to zero and restores the caller's floating-point mode on return. Fed a stereo block of subnormal samples (`1e-40f`) with the gain at 1, no output sample is subnormal, and the denormal mode (`juce::FloatVectorOperations::areDenormalsDisabled()`) is the same before and after the call.
- **DSP-10** CI has a second job that builds the `Tests` target as a Debug build with AddressSanitizer (`-DDUKKO_ASAN=ON`, which adds `-fsanitize=address -fno-omit-frame-pointer` to compile and link of the plugin code and the tests) and runs `ctest`; an ASan report fails the job. It runs on every push and pull request next to the existing build job.
- **DSP-08** CPU footprint <1% per instance on M-series at 48 kHz / 256-sample buffer
