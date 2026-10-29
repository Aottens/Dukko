# Module spec: state (STAT)

Status: **STAT-01 approved 2026-10-28 (VS1-01)**; **STAT-02, 04, 05, 06 approved 2026-10-29 (VS1-03)**; STAT-03 is a draft carried over from the 2026-05-03 requirements, approved when its task starts.

## Format

The state is the APVTS `juce::ValueTree` of type `DukkoState`, written as JUCE's binary ValueTree
stream, as before VS1-03 (projects the owner saved before VS1-03 hold exactly this). Each
parameter is a `PARAM` child with `id` and `value`. The root carries an integer property
`version`; the format at VS1-03 is version `1`. State that lands later (user curve geometry, A/B
snapshots, sync state) goes in as child trees of `DukkoState`; a change an older reader would
misread raises `version` and adds a migration step (STAT-06).

## Acceptance criteria

- **STAT-01** Every host parameter has a string id and version hint 1 (`juce::ParameterID {id, 1}`); ids so far: `depth`, `mix`, `bypass`, `division` (VS2-01). An id never changes or gets reused once released.
- **STAT-02** Saved state is a `DukkoState` tree with `version="1"` and one `PARAM` child per parameter (`depth`, `mix`, `bypass`, `division`) holding its current value; loading it into a fresh instance restores every parameter value (to float precision).
- **STAT-03** Factory curves serialize by index (not embedded), so future factory updates ship transparently (draft).
- **STAT-04** In Bitwig, as CLAP and as VST3: set depth, mix and bypass away from their defaults, save, close the project, reopen it: all three values are back. A project saved before VS1-03 (no `version`) opens with its values. (Manual checklist; extended with curve, A/B and sync state when those exist.)
- **STAT-05** Loading is deterministic and idempotent: the same data gives the same parameter values and the same saved state whatever the instance held before, and loading twice equals loading once. A parameter missing from the data, or without a value, gets its default; a value out of range is clamped to the range; unknown children and properties are ignored and dropped. Data that is not a `DukkoState` tree (empty, garbage, other type) changes nothing.
- **STAT-06** Load runs the data through one migration step per version, in order, up to the current version, before applying it. Data without `version` is version 0 (the pre-VS1-03 format) and its step to 1 only adds `version="1"`: it loads with its values and saves back as version 1. Data with a version newer than this build knows is loaded best-effort by STAT-05 (known parameters applied, the rest dropped) and saved back as version 1.
