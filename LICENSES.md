# Dukko — Third-Party Licenses

This file tracks all third-party dependencies bundled with or linked into Dukko,
plus CI tools that run against the plugin binary. Re-verify at every dep bump and
at any commercial-release decision.

Last updated: 2026-05-03

## Bundled / Linked Dependencies

| Dep | Version pin | License | Source URL | License text |
|-----|-------------|---------|------------|--------------|
| JUCE | 8.0.12 | JUCE 8 Personal EULA (free commercial use ≤$50k/yr) | https://github.com/juce-framework/JUCE | https://juce.com/legal/juce-8-licence/ |
| clap-juce-extensions | `main` @ `e8de9e8571626633b8541a54c2406fccc4272767` | MIT | https://github.com/free-audio/clap-juce-extensions | https://github.com/free-audio/clap-juce-extensions/blob/main/LICENSE |
| chowdsp_utils | v2.4.0 | BSD-3-Clause | https://github.com/Chowdhury-DSP/chowdsp_utils | https://github.com/Chowdhury-DSP/chowdsp_utils/blob/main/LICENSE |
| CPM.cmake | bundled at `cmake/CPM.cmake` (downloads v0.42.0 at configure time) | MIT | https://github.com/cpm-cmake/CPM.cmake | https://github.com/cpm-cmake/CPM.cmake/blob/master/LICENSE |
| CLAP headers | transitive via clap-juce-extensions (CLAP 1.2.7) | MIT | https://github.com/free-audio/clap | https://github.com/free-audio/clap/blob/main/LICENSE |
| melatonin_inspector | `main` @ `9e91e4e3d6cc41688c8d2108ef7ed33c1a90dcc9` | MIT | https://github.com/sudara/melatonin_inspector | https://github.com/sudara/melatonin_inspector/blob/main/LICENSE |
| Catch2 | v3.8.1 (transitive: pulled by `cmake/Tests.cmake` for the Tests + Benchmarks targets) | BSL-1.0 | https://github.com/catchorg/Catch2 | https://github.com/catchorg/Catch2/blob/devel/LICENSE.txt |
| sudara/cmake-includes (vendored as `cmake/Dukko*.cmake` + `cmake/{JUCEDefaults,SharedCodeDefaults,Assets,XcodePrettify,Tests,Benchmarks,GitHubENV}.cmake`) | `main` @ ingestion (D-07: commit divergence — Dukko owns these files outright from plan 02 forward) | MIT | https://github.com/sudara/cmake-includes | https://github.com/sudara/cmake-includes (no LICENSE file in repo; same MIT terms as sibling https://github.com/sudara/pamplejuce/blob/main/LICENSE) |

## CI-Only Tools (not linked into Dukko; license does not affect plugin)

| Tool | Version | License | Note |
|------|---------|---------|------|
| pluginval | 1.0.4 | GPLv3 | Runs against the plugin binary, not linked in. GPLv3 does not taint Dukko. |
| clap-validator | 0.3.2 | MIT | Runs against the plugin binary, not linked in. |
| Steinberg VST3 validator | bundled in JUCE 8 SDK fork | Steinberg proprietary | Runs against the plugin binary, not linked in. |

## GPLv3 note on pluginval

pluginval is licensed GPLv3. It is a test-runner that executes the plugin
binary in a subprocess. It does not link into the Dukko binary and does not
create a derivative work. Dukko's license is unaffected — the GPLv3 license
of the test tool does not taint the plugin. This note documents the reasoning
in writing so any future commercial-release audit can confirm it without
re-deriving the analysis.

## Maintenance contract (QUAL-05)

- Adding a new CPM dependency? Add a row to **Bundled / Linked Dependencies**
  in the same commit that adds the `CPMAddPackage` block.
- Adding a new CI-only tool? Add a row to **CI-Only Tools**.
- Bumping a pin? Update the version cell + re-verify the license URL still
  resolves and still says what we say it says.
- Plan 03 wires a CI step that fails the build if any dependency dir under
  `build/_deps/` lacks a matching row in this file (the QUAL-05 enumeration
  check). Keep this file in sync with `CMakeLists.txt`.

## Source ingestion record (informational, not a runtime dep)

The repository scaffold itself was ingested from the Pamplejuce template
once on 2026-05-03 and committed as Dukko's own files (D-07 commit divergence;
no submodule / no upstream tracking). Recorded here for license traceability:

- **sudara/pamplejuce** @ `c045cfb49a942422fb94fe8203f0266e0c389f5c` — MIT —
  https://github.com/sudara/pamplejuce/blob/main/LICENSE
