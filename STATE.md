# Dukko — State

_Last updated: 2026-10-29 (VS1-05 percent text)_

This file is the handoff: where we are, what is next, what waits. It is not a log.

## Current focus
v1, slice 1: parameters, bypass and state recall. The reset (VS0-01) is merged; depth and mix
(VS1-01) and bypass with a 10 ms crossfade (VS1-02) live in `source/dsp/Duck.h`, fed a zero
curve until slice 2, so none of them is audible yet; the click check by ear is part of VS2-04.
Depth and mix read and parse as whole percentages (VS1-05, DSP-09; Bitwig run passed).

## Next
1. VS1-03: approve STAT-02/05/06 in `specs/modules/state.md` (gate 1), then versioned state.

## Waiting for a run in Bitwig (gate 3, owner)
- none

## Blocked
- none

## Owner decisions open
- The module specs are carried over unreviewed; each is approved when its first task starts.
