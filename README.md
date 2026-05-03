# Dukko

Tempo-locked, click-and-go ducking for Bitwig on Apple Silicon — VST3 + CLAP.

[![Build & Validate](https://github.com/Aottens/Dukko/actions/workflows/build_and_test.yml/badge.svg)](https://github.com/Aottens/Dukko/actions/workflows/build_and_test.yml)

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
cmake --build build --config Release --target Dukko_VST3 --target Dukko_CLAP
```

The build auto-installs both the VST3 and CLAP bundles into
`~/Library/Audio/Plug-Ins/{VST3,CLAP}/` on macOS thanks to JUCE's
`COPY_PLUGIN_AFTER_BUILD TRUE`. Restart your DAW (or trigger a plugin re-scan)
to pick up the new build.

Phase 1 ships the build foundation + CI: the tree builds end-to-end on macOS
arm64 and produces both VST3 and CLAP bundles, gated by pluginval strict-10 +
clap-validator on every push. There is no DSP yet — the plugin instantiates
silently. Phase 2 wires up parameters, state, and the ducker DSP.

## Status

v1 is in active development. See `.planning/ROADMAP.md` for the phase plan and
`.planning/STATE.md` for the current position.
