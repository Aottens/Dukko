# Dukko

Tempo-synced volume ducking for Bitwig on Apple Silicon, as VST3 and CLAP. Drop it on a track,
pick a curve, get a tight, musical pump locked to the host tempo.

## Status
v1, slice 0: a lean passthrough skeleton that builds VST3 + CLAP and is gated in CI by tests,
pluginval (strictness 10) and clap-validator. Next: depth, wet/dry, bypass and state recall
(slice 1). Plan in `BACKLOG.md`, handoff in `STATE.md`.

## What works today
- Builds as native arm64 VST3 and CLAP and installs itself into `~/Library/Audio/Plug-Ins/`.
- Passes audio through unchanged.

## Build and run

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build && ctest --test-dir build
```

Rescan plugins in Bitwig to pick up the new build.

## How work happens
Spec → test → code → review, one task per PR; see `CLAUDE.md`. Scope: `specs/PRD.md`.
