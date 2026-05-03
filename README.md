# Dukko

Tempo-locked, click-and-go ducking for Bitwig on Apple Silicon — VST3 + CLAP.

[![Build & Validate](https://github.com/<your-org>/Dukko/actions/workflows/build_and_test.yml/badge.svg)](https://github.com/<your-org>/Dukko/actions/workflows/build_and_test.yml)

## What this is

Dukko is a tempo-synced volume-ducking effect — drop it on a track, pick a curve,
get a tight, musical pumping effect. No fiddling, no sidechain routing, no
trigger setup. Built first for personal use in Bitwig on Apple Silicon, with
Windows and a possible commercial release on the table once v1 lands.

The bar is "strak en bruikbaar voor productie" — own visual identity, readable
curves, solid host integration (Bitwig is the strict acceptance host), no
audible glitches under normal use. Not showroom polish; not a toy either.

## Build

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The build auto-installs the VST3 bundle into `~/Library/Audio/Plug-Ins/VST3/` on
macOS thanks to JUCE's `COPY_PLUGIN_AFTER_BUILD TRUE`. Restart your DAW (or
trigger a plugin re-scan) to pick up the new build.

Plan 01 ships the rename / scaffold pass only — the tree is not yet buildable
end-to-end. Plan 02 wires up JUCE 8 + clap-juce-extensions + chowdsp_utils via
CPM and is what makes `cmake -B build` actually succeed.

## Status

v1 is in active development. See `.planning/ROADMAP.md` for the phase plan and
`.planning/STATE.md` for the current position.
