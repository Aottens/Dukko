# Dukko — State

_Last updated: 2026-10-28 (VS1-01 depth and mix)_

This file is the handoff: where we are, what is next, what waits. It is not a log.

## Current focus
v1, slice 1: parameters, bypass and state recall. The reset (VS0-01) is merged; depth and mix
(VS1-01) exist as smoothed parameters with the gain law in `source/dsp/Duck.h`, fed a zero
curve until slice 2.

## Next
1. Owner: run `tests/manual/bitwig-load.md` (VS0-01, gate 3).
2. VS1-02: approve DSP-04 (bypass crossfade) in `specs/modules/dsp.md` (gate 1), then build it.

## Waiting for a run in Bitwig (gate 3, owner)
- VS0-01, `tests/manual/bitwig-load.md`.

## Blocked
- none

## Owner decisions open
- The module specs are carried over unreviewed; each is approved when its first task starts.
