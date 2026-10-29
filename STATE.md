# Dukko — State

_Last updated: 2026-10-29 (VS2-03 curve model)_

This file is the handoff: where we are, what is next, what waits. It is not a log.

## Current focus
v1, slice 2: tempo sync on a fixed curve. Slice 1 (parameters, bypass, state recall) is complete. The reset (VS0-01) is merged; depth and mix
(VS1-01) and bypass with a 10 ms crossfade (VS1-02) live in `source/dsp/Duck.h`; the click
check by ear is part of VS2-04.
Depth and mix read and parse as whole percentages (VS1-05, DSP-09; Bitwig run passed).
State is `DukkoState version="1"`, loaded deterministically through a migration hook; pre-VS1-03
state loads as version 0 (VS1-03, STAT-02/05/06; Bitwig run passed). A test-only allocation
guard proves `processBlock` never allocates, a test proves denormals are flushed, and CI runs the
tests under AddressSanitizer (VS1-04, DSP-06/07/10).
`source/dsp/Sync.h` derives the phase per sample from the block's PPQ, tempo and the new
`division` parameter (13 entries, default 1/4) and is inactive on unusable or stopped transport
(VS2-01, SYNC-01/02/05). The processor reads the host play head into `dsp::Transport` and a
fixed raised-cosine curve ducks per sample, fully in on each cycle start; the duck ramps in and
out over 10 ms when the transport starts or stops (VS2-02, DSP-01). Dukko is audible from here; the Bitwig run passed.
The curve is now a point-and-tension model (`source/dsp/Curve.h`), rendered to a 1024-entry LUT and
handed to the audio thread through a lock-free triple buffer with a 10 ms crossfade on change; the
default `Curve::sine()` stands in for the fixed cosine (VS2-03, CURV-01..03). Nothing publishes a new
curve yet; the click check by ear comes with the editor (UI-07).

## Next
1. VS2-04: the owner's Bitwig run for drift, loops, tempo automation and bypass clicks (SYNC-03, 04, DSP-04, STAT-04).

## Waiting for a run in Bitwig (gate 3, owner)
- none

## Blocked
- none

## Owner decisions open
- The module specs are carried over unreviewed; each is approved when its first task starts (curve: CURV-01..03 approved 2026-10-29).
