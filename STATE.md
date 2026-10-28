# Dukko — State

_Last updated: 2026-10-28 (VS0-01 Bitwig run passed)_

This file is the handoff: where we are, what is next, what waits. It is not a log.

## Current focus
v1, slice 1: parameters, bypass and state recall. The reset (VS0-01) is merged; depth and mix
(VS1-01) exist as smoothed parameters with the gain law in `source/dsp/Duck.h`, fed a zero
curve until slice 2.

## Next
1. VS1-02: approve DSP-04 (bypass crossfade) in `specs/modules/dsp.md` (gate 1), then build it.
2. VS1-05: Depth and Mix read `30 %` instead of `30.0000000` (from the VS0-01 run).

## Waiting for a run in Bitwig (gate 3, owner)
- none

## Blocked
- none

## Owner decisions open
- The module specs are carried over unreviewed; each is approved when its first task starts.
