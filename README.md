# Dukko

Tempo-synced volume ducking for Bitwig on Apple Silicon, as VST3 and CLAP. Drop it on a track,
pick a curve, get a tight, musical pump locked to the host tempo.

## Status
Early v1. Dukko loads in Bitwig as CLAP and VST3 and has Depth, Mix and Bypass with readable
values that survive saving the project, but does not duck yet: the tempo-synced curve comes next.
Plan in `BACKLOG.md`, handoff in `STATE.md`.

## What works today
- Builds as native arm64 VST3 and CLAP and installs itself into `~/Library/Audio/Plug-Ins/`.
- Passes audio through unchanged.
- Depth and Mix (0–100 %) as automatable host parameters, ramped over 20 ms, saved with the project; they read
  `30 %` and typing `30` sets 30 %.
- Bypass as the plugin's own host bypass, crossfading over 10 ms.
- Every setting comes back when a project is reopened, including projects saved with earlier builds.

## Build and run

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build && ctest --test-dir build
```

Rescan plugins in Bitwig to pick up the new build.

## How work happens
Spec → test → code → review, one task per PR; see `CLAUDE.md`. Scope: `specs/PRD.md`.
