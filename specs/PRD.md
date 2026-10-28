# Dukko — PRD (phase 1 = v1)

Phase 1 is everything in `specs/modules/*.md`. The criteria there are the scope; this file says
what is in and what is out. Ideas outside it go to `IDEAS.md`.

## v1: "bruikbaar voor productie" in Bitwig on Apple Silicon
1. **Build** (BUILD): VST3 + CLAP, native arm64, CI with pluginval and clap-validator.
2. **Audio-thread-safe passthrough with parameters and state recall** (DSP-02..07, STAT-01..06):
   depth, wet/dry, click-free bypass; Bitwig save → close → reopen restores everything.
3. **Tempo-sync engine** (DSP-01, SYNC-01..05, CURV-01..03): a curve locked to host PPQ,
   phase derived per block, published to the audio thread without locks.
4. **Curve display and factory curves** (CURV-04, 06, 07, UI-01..03): one screen, live playhead,
   resizable 100/125/150 %.
5. **One editable user curve** (CURV-05, UI-04..07, 09, 10), saved with the project.
6. **A/B compare and validation** (UI-08, QUAL-02..04, DSP-08).

## Later (v2)
Windows port; codesigning, notarisation and installer; multiple curve slots, curve
import/export, randomise; tempo offset, MIDI trigger, audio trigger, external sidechain;
multiband and mid/side; oversampling; zoom beyond 150 % and free resize.

## Out of scope
AU, AAX, standalone app; modulation matrix; MIDI output; cloud presets; built-in
compressor/EQ/saturation; AI features; an own undo system (the host's undo is used).
