# Module spec: sync (SYNC)

Status: **SYNC-01, 02, 05 approved 2026-10-29 (VS2-01)**; SYNC-03, 04 are drafts carried over from the 2026-05-03 requirements, approved when their task starts (gate 1).

## Definitions
- **Position** `ppq`: the host's position at the first sample of the block, in quarter notes (JUCE `getPpqPosition`, VST3 `ProcessContext.projectTimeMusic`, CLAP `song_pos_beats`). It may be negative (pre-roll, count-in).
- **Cycle length** `L`: the length of one curve cycle in quarter notes, set by the division (SYNC-02).
- **Phase** `p` ∈ [0, 1): where sample `i` of the block sits in the cycle, `p = frac((ppq + i × bpm / (60 × sampleRate)) / L)` with `frac(x) = x − floor(x)`, kept below 1 (a result that rounds to 1.0 is 0).
- **Active**: whether the transport gives a usable position (SYNC-05). The curve stage (VS2-02) ducks only while active.

## Acceptance criteria

- **SYNC-01** The phase of every sample is computed from the current block's `ppq`, `bpm` and the sample rate with the formula above; nothing is carried over from earlier blocks, so the same block input always gives the same phases, whatever came before. At 48 kHz, 120 bpm, division 1/4: `ppq` 0 gives phase 0 at sample 0 and 0.5 at sample 12000; `ppq` 1.25 gives 0.25; `ppq` −0.25 gives 0.75; `ppq` −1e-17 gives a phase in [0, 1). After a block at `ppq` 7.9, a block at `ppq` 0 (loop back) starts at phase 0. Over 5 minutes of 256-sample blocks, with `ppq` per block taken from the sample count, every block-start phase equals `frac(ppq / L)` within 1e-9.
- **SYNC-02** Host parameter `division` (choice, string id `division`, version hint 1), default `1/4`, with these 13 entries in this order, shown with these texts: `1/1`, `1/2`, `1/2.`, `1/2T`, `1/4`, `1/4.`, `1/4T`, `1/8`, `1/8.`, `1/8T`, `1/16`, `1/16.`, `1/16T`. Straight `1/n` has `L = 4 / n` (1/1 = 4, 1/4 = 1, 1/16 = 0.25), dotted is `1.5 × L`, triplet is `2/3 × L`. At 48 kHz, 120 bpm and `ppq` 0, the phase reaches 0.5 at sample 8000 for 1/4T, 12000 for 1/4 and 18000 for `1/4.`. The entries and their order are permanent like the id (hosts store automation and state by index); new divisions can only be appended. A division change takes effect at the next block (smoothing the jump is SYNC-04).
- **SYNC-03** Curve is sample-accurate over a 5-minute Bitwig session (zero drift)
- **SYNC-04** Loop points, playhead jumps, and tempo automation produce no clicks or phase glitches
- **SYNC-05** The sync is inactive, and every phase is 0, when any of these holds: no play head or no position this block, `ppq` missing or not finite, `bpm` missing, not finite or below 1, the transport is stopped, or the sample rate is not above 0. No input gives a NaN or infinite phase. `bpm` exactly 1 with a valid position while playing is active. While inactive the plugin does not duck (the curve stage treats it as curve value 0, VS2-02).
