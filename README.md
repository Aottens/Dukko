# Dukko

Tempo-synced volume ducking for Bitwig on Apple Silicon, as VST3 and CLAP. Drop it on a track,
pick a curve, get a tight, musical pump locked to the host tempo.

## Status
v1, slice 1: depth, mix and bypass exist as smoothed host parameters; they do nothing audible
yet, because the duck curve arrives in slice 2. Next: readable values (VS1-05) and versioned
state recall (VS1-03). Plan in `BACKLOG.md`, handoff in `STATE.md`.

## What works today
- Builds as native arm64 VST3 and CLAP and installs itself into `~/Library/Audio/Plug-Ins/`.
- Passes audio through unchanged.
- Depth and Mix (0–100 %) as automatable host parameters, ramped over 20 ms, saved with the project.
- Bypass as the plugin's own host bypass, crossfading over 10 ms.

## Build and run

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build && ctest --test-dir build
```

Rescan plugins in Bitwig to pick up the new build.

## How work happens
Spec → test → code → review, one task per PR; see `CLAUDE.md`. Scope: `specs/PRD.md`.
