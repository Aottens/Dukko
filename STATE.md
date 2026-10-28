# Dukko — State

_Last updated: 2026-10-28 (reset onto the CMDMX way of working, VS0-01)_

This file is the handoff: where we are, what is next, what waits. It is not a log.

## Current focus
v1, slice 0: the repo is stripped to a lean JUCE skeleton (passthrough VST3 + CLAP, Catch2,
CI with pluginval and clap-validator). The old GSD planning is gone; its requirements live on,
ids intact, in `specs/modules/`.

## Next
1. Owner: run `tests/manual/bitwig-load.md` (VS0-01, gate 3).
2. VS1-01: approve `specs/modules/dsp.md` DSP-02/03/05 and `state.md` STAT-01 (gate 1), then
   depth and wet/dry.

## Waiting for a run in Bitwig (gate 3, owner)
- VS0-01, `tests/manual/bitwig-load.md`.

## Blocked
- none

## Owner decisions open
- The module specs are carried over unreviewed; each is approved when its first task starts.
