# Dukko

A VST3 + CLAP plugin for tempo-synced volume ducking: drop it on a track, pick a curve, get a
tight, musical pump locked to the host tempo. No sidechain routing. Reference product in the
category: Cableguys/Nicky Romero Kickstart.

## Owner
Aernout Ottens: product owner, reviewer, tests in Bitwig. Claude builds.

## Core value
Tempo-locked, click-and-go ducking that sounds and feels good in production. If everything else
fails, dropping it on a track and getting a musical pump must just work.

## Scope
- v1 ("bruikbaar voor productie"): `specs/PRD.md`, criteria in `specs/modules/`.
- v2 and out of scope: also in `specs/PRD.md`. Unplaced ideas: `IDEAS.md`.

## Constraints
- Apple Silicon (arm64) macOS first, native; Windows x64 must stay reachable from the same code.
- VST3 and CLAP; no AU, AAX or standalone.
- Bitwig is the strict acceptance host (state recall, automation).
- Under 1 % CPU per instance on an M-series Mac at 48 kHz / 256 samples.
- Fully offline: no telemetry, no licence server.
- A commercial release stays possible: no GPL code linked in (JUCE 8 Personal licence; re-check
  the JUCE and Steinberg terms when a release is decided).

## Stack
C++20, JUCE 8, clap-juce-extensions, Catch2, CMake + Ninja, GitHub Actions on `macos-14`.
