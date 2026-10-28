# Module spec: quality (QUAL)

Status: **draft, carried over from the 2026-05-03 requirements; the owner approves each criterion when its task starts (gate 1).**

## Acceptance criteria

- **QUAL-01** pluginval at strictness level 10 passes on every CI build (no failures, no warnings beyond known acceptable)
- **QUAL-02** Steinberg VST3 SDK validator passes
- **QUAL-03** Plugin survives Bitwig stress tests: rapid project switching, sample-rate change mid-session, automation-heavy track
- **QUAL-04** AddressSanitizer build runs clean for at least one full Bitwig session
- **QUAL-05** Every third-party dependency and tool is listed with version and licence under *Dependencies* in `ARCHITECTURE.md`, updated in the PR that adds it.
