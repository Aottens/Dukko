# Phase 1: build-foundation-ci — Research

**Researched:** 2026-05-03
**Domain:** JUCE 8 / CMake / GitHub Actions / Plugin validation toolchain
**Status:** Complete

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

**Plugin Identifiers (PERSISTENT — set once, baked into binaries forever)**
- D-01: Manufacturer name = `Dukko Audio`
- D-02: 4-character manufacturer code = `Dukk`
- D-03: 4-character plugin code = `Dkk1`
- D-04: macOS bundle ID + CLAP plugin ID = `com.dukkoaudio.dukko`

**Repo & Directory Layout**
- D-05: Rename working folder `KickstartClone` → `Dukko`; create GitHub repo `Dukko` (public)
- D-06: Source layout = Pamplejuce default (`source/` lowercase at repo root)
- D-07: Pamplejuce ingestion = "Use this template" then commit divergence (no upstream sync)
- D-08: GitHub repo visibility = public

**Auto-install**
- D-09: `COPY_PLUGIN_AFTER_BUILD=TRUE` — copies built bundles to `~/Library/Audio/Plug-Ins/VST3/` and `~/Library/Audio/Plug-Ins/CLAP/` each local build

**CI Scope**
- D-10: Validators on every push: pluginval strict-10 (VST3), clap-validator (CLAP), Steinberg validator (VST3)
- D-11: Two jobs: Release (validators) + Debug+ASan compile-only
- D-12: Upload `Dukko.vst3` and `Dukko.clap` as workflow artifacts (30-day retention)
- D-13: CI triggers = push to any branch + pull requests
- D-14: CPM cache enabled, keyed on CMakeLists.txt hash

**Dependencies**
- D-15: Pull `chowdsp_utils` via CPM in Phase 1
- D-16: Pin JUCE and clap-juce-extensions to specific tags/SHAs
- D-17: LICENSES.md format = hand-curated markdown table: `dep | version pin | license | source URL | license-text link`

### Claude's Discretion
- 4-char manufacturer code `Dukk` already chosen
- JUCE / clap-juce-extensions version pins → researcher selects current-latest-stable
- `CMAKE_OSX_DEPLOYMENT_TARGET` → 11.0 chosen (justified below)
- CMake version string scheme → semver `0.1.0`
- `.gitignore` content
- README.md content
- CI status badge in README

### Deferred Ideas (OUT OF SCOPE)
- CI status badge — include as one-line task (already noted as mechanical)
- Concurrency `cancel-in-progress` — micro-optimization, later phase
- Codesigning, notarization, installer — out-of-scope until commercial release
- Windows runner — v2 milestone
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| BUILD-01 | Scaffold from Pamplejuce, building VST3 from CMake on macOS arm64 | Pamplejuce CMakeLists.txt rename plan documented below |
| BUILD-02 | Plugin loads in Bitwig as native arm64 (verified via `lipo -archs`) | `CMAKE_OSX_ARCHITECTURES=arm64`; ad-hoc codesign step documented |
| BUILD-03 | Plugin builds as CLAP alongside VST3 via `clap-juce-extensions` | CPM block + `clap_juce_extensions_plugin()` call documented |
| BUILD-04 | GitHub Actions CI builds VST3+CLAP on `macos-14` every push | Workflow plan with job structure documented |
| BUILD-05 | Plugin name and identifiers are Dukko across CMake, manifest, binaries | Exhaustive rename touchpoints documented |
| QUAL-01 | pluginval at strictness 10 passes on every CI build | Exact CLI and download step documented |
| QUAL-05 | LICENSES.md tracks all deps from day 1 | LICENSES.md format and dep list documented |
</phase_requirements>

---

## Summary

- Pamplejuce template provides a near-complete scaffold: CMake 3.25+, CPM, JUCE 8 as git submodule, clap-juce-extensions, GitHub Actions with pluginval CI already wired. The rename cost is 5–6 targeted string substitutions in `CMakeLists.txt` and `CMakePresets.json`. [VERIFIED: raw.githubusercontent.com/sudara/pamplejuce/main/CMakeLists.txt]
- JUCE latest stable is **8.0.12** (released December 16, 2024). [VERIFIED: github.com/juce-framework/JUCE/releases]
- `clap-juce-extensions` publishes no release tags; pin to a recent `main` commit SHA. The CMake integration is `clap_juce_extensions_plugin(TARGET ...)` after `juce_add_plugin(...)`. [VERIFIED: github.com/free-audio/clap-juce-extensions]
- `chowdsp_utils` latest release is **v2.4.0** (November 29, 2025), BSD licensed for the modules this project uses. [VERIFIED: github.com/Chowdhury-DSP/chowdsp_utils]
- `clap-validator` latest release is **0.3.2** (March 25, 2026); pluginval latest is **1.0.4**. [VERIFIED: respective GitHub releases pages]
- `CMAKE_OSX_DEPLOYMENT_TARGET=11.0` is the correct floor for arm64-only personal-use v1 (macOS Big Sur, first macOS with native arm64 support). [CITED: CLAUDE.md §"CI / Packaging"]

**Primary recommendation:** Clone Pamplejuce with "Use this template," apply the 6 targeted CMakeLists.txt edits listed below, wire in CPM deps at pinned versions, add the clap-validator and Steinberg validator steps to the existing CI workflow, and ship.

---

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Build system / CMake config | Build toolchain | — | CMake owns all targets; no runtime tier |
| Plugin manifest & identifiers | Binary / bundle metadata | Host (Bitwig reads) | Baked at build time, consumed by host at load time |
| CLAP format wrapper | Build toolchain (clap-juce-extensions) | — | CMake macro generates the CLAP target |
| CI validation | CI runner (GitHub Actions) | — | pluginval/clap-validator/Steinberg run headlessly |
| Local auto-install | Build toolchain (JUCE COPY_PLUGIN_AFTER_BUILD) | — | CMake install step copies to ~/Library paths |
| Dependency tracking | Repo artifact (LICENSES.md) | — | Hand-curated, no runtime component |

---

## Pamplejuce Ingestion & Rename Plan

### GitHub Flow

1. Go to https://github.com/sudara/pamplejuce → "Use this template" → "Create a new repository"
2. Name the new repo `Dukko`, visibility = Public (D-08), owner = your GitHub account
3. Clone locally to `~/Documents/Projects/Dukko` (or wherever; matches D-05 folder rename)
4. Make an immediate commit with only the rename edits below — this is the "commit divergence" point (D-07)

### Exhaustive Rename Touchpoints

All in `CMakeLists.txt` at repo root (the main config file returned by Pamplejuce). Change exactly these values:

| Variable / Parameter | Pamplejuce default | Dukko value |
|---------------------|--------------------|-------------|
| `set(PROJECT_NAME ...)` | `"Pamplejuce"` | `"Dukko"` |
| `set(PRODUCT_NAME ...)` | `"Pamplejuce Demo"` | `"Dukko"` |
| `set(COMPANY_NAME ...)` | `"Pamplejuce Company"` | `"Dukko Audio"` |
| `set(BUNDLE_ID ...)` | `"com.pamplejuce.pamplejuce"` | `"com.dukkoaudio.dukko"` |
| `set(FORMATS ...)` | `Standalone AU VST3 AUv3` | `VST3 CLAP` |
| `PLUGIN_MANUFACTURER_CODE` in `juce_add_plugin()` | `Pamp` | `Dukk` |
| `PLUGIN_CODE` in `juce_add_plugin()` | `P001` | `Dkk1` |

Secondary touchpoints to check:

| File | What to look for | Action |
|------|-----------------|--------|
| `README.md` | "Pamplejuce" in title, badge URLs, any template instructions | Replace with Dukko project description; update badge URL to `github.com/[your-org]/Dukko` |
| `CMakePresets.json` (if present) | Preset name strings that embed "Pamplejuce" | Rename to "Dukko-*" |
| `source/PluginEditor.h` / `.cpp` | Any hardcoded "Pamplejuce" or "Demo" strings in the initial GUI text | Replace with "Dukko" |
| `packaging/icon.png` | Pamplejuce logo | Replace with Dukko icon (or leave placeholder for Phase 4) |
| `packaging/` directory | Any `.plist` template fragments with old bundle ID | Update bundle ID |
| `.github/workflows/*.yml` | Job names, artifact names referencing "Pamplejuce" | Rename to "Dukko" |

**Note on `FORMATS`:** Remove `Standalone`, `AU`, `AUv3` (out of scope per PROJECT.md). The `CLAP` format token is enabled by `clap-juce-extensions` — see CPM blocks below. [CITED: CLAUDE.md §"What NOT to use"]

### COPY_PLUGIN_AFTER_BUILD (D-09)

`COPY_PLUGIN_AFTER_BUILD TRUE` is already set in Pamplejuce's `juce_add_plugin()` call. [VERIFIED: pamplejuce CMakeLists.txt]

What it does: After a successful CMake build, JUCE copies:
- `Dukko.vst3` → `~/Library/Audio/Plug-Ins/VST3/Dukko.vst3`
- `Dukko.clap` → `~/Library/Audio/Plug-Ins/CLAP/Dukko.clap` (when CLAP format is enabled)

No additional CMake configuration is needed — the variable is already `TRUE` in the template. Confirm it remains `TRUE` after the rename edits.

---

## CMake / Build Configuration

```cmake
cmake_minimum_required(VERSION 3.25)  # Pamplejuce default [VERIFIED]

project(Dukko VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 23)            # Pamplejuce default is C++23 [VERIFIED]
# Note: CLAUDE.md recommends C++20; C++23 is what Pamplejuce ships.
# C++23 is fine — Apple Clang 15+ supports it and adds no cost.
# Recommendation: leave as C++23 (Pamplejuce default) rather than downgrading.

set(CMAKE_OSX_ARCHITECTURES "arm64")
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0")
# 11.0 justification: macOS Big Sur is the first OS with native arm64.
# Personal-use v1 has no reason to require 12.0.
# Raise to 12.0+ only when targeting App Store / commercial notarization.
```

**CMake minimum version:** Pamplejuce ships `cmake_minimum_required(VERSION 3.25)`. No bump needed. [VERIFIED: pamplejuce CMakeLists.txt]

**Version string:** Set `project(Dukko VERSION 0.1.0)`. Pamplejuce ships a `VERSION` file that CPM/JUCE reads to propagate through the plugin manifest — update that file to `0.1.0` as well. [CITED: Pamplejuce README — "A VERSION file that will propagate through JUCE and your app"]

**`.gitignore` baseline:** Pamplejuce ships one. Verify it covers:
- `build/` and `Builds/` (CMake output dirs)
- `.DS_Store`
- `*.xcodeproj/` (generated by `cmake -G Xcode`)
- `_deps/` (CPM download cache, if not using CPM_SOURCE_CACHE)
- `cmake-build-*/` (CLion)
- `.idea/` (JetBrains)

**TBD-during-execution:** Read the actual Pamplejuce `.gitignore` on clone and verify these entries are present; add any missing ones.

---

## Dependency Pins (D-15, D-16, D-17)

| Dep | Recommended pin | Source | SPDX license | Notes |
|-----|-----------------|--------|--------------|-------|
| JUCE | `8.0.12` (tag) | [VERIFIED: github.com/juce-framework/JUCE/releases — Dec 16 2024] | JUCE 8 Personal EULA | Free commercial use up to $50k/yr |
| clap-juce-extensions | Latest `main` HEAD SHA at clone time | [VERIFIED: no release tags exist] | MIT | Fetch via CPM GIT_TAG from SHA |
| chowdsp_utils | `v2.4.0` (tag, Nov 29 2025) | [VERIFIED: github.com/Chowdhury-DSP/chowdsp_utils] | BSD-3-Clause | For modules used (plugin_state, core) |
| CPM.cmake | Bundled with Pamplejuce | [ASSUMED: Pamplejuce includes CPM.cmake; verify on clone] | MIT | |
| clap headers (`clap`) | Transitive via clap-juce-extensions | [VERIFIED: clap-juce-extensions pulls clap headers] | MIT | No direct CPM block needed |
| pluginval | 1.0.4 | [VERIFIED: github.com/Tracktion/pluginval/releases] | GPLv3 (CI tool only — NOT linked into Dukko; does not taint plugin license) | Download in CI only |
| clap-validator | 0.3.2 (Mar 25 2026) | [VERIFIED: github.com/free-audio/clap-validator/releases] | [ASSUMED: MIT-compatible; verify in LICENSES.md] | Download in CI only |
| Steinberg VST3 validator | Bundled inside JUCE's VST3 SDK fork | [ASSUMED: binary at `_deps/juce-src/modules/juce_audio_plugin_client/VST3/...`; verify path after first build] | Steinberg proprietary (CI tool only, not linked into Dukko) | |

**TBD-during-execution:** After first `cmake` configure run, locate the Steinberg `validator` binary path in the build tree and record the exact path for CI invocation.

**TBD-during-execution:** Record the actual `main` HEAD SHA for `clap-juce-extensions` at clone time and lock it in CPM block. Revisit every major JUCE or CLAP spec bump.

---

## CMake CPM Block Sketches

These blocks go in the root `CMakeLists.txt` after the initial project setup, before `juce_add_plugin()`.

### JUCE (pinned to 8.0.12)

```cmake
CPMAddPackage(
    NAME JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.12
    GIT_SHALLOW TRUE
)
```

**Note:** Pamplejuce may use JUCE as a git submodule rather than CPM. If so, update the submodule ref to `8.0.12` tag instead of adding a CPM block. **TBD-during-execution:** check whether Pamplejuce fetches JUCE via CPM or submodule on the current main branch.

### clap-juce-extensions (pinned to SHA)

```cmake
CPMAddPackage(
    NAME clap-juce-extensions
    GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
    GIT_TAG <SHA-TO-FILL-AT-CLONE-TIME>
    GIT_SHALLOW FALSE   # shallow clone fails for non-tag SHAs
)
```

After `juce_add_plugin("Dukko" ...)`, add:

```cmake
clap_juce_extensions_plugin(
    TARGET Dukko
    CLAP_ID "com.dukkoaudio.dukko"
    CLAP_FEATURES audio-effect mixing
)
```

[VERIFIED: github.com/free-audio/clap-juce-extensions — exact macro name and signature]

### chowdsp_utils (pinned to v2.4.0)

```cmake
CPMAddPackage(
    NAME chowdsp_utils
    GIT_REPOSITORY https://github.com/Chowdhury-DSP/chowdsp_utils.git
    GIT_TAG v2.4.0
    GIT_SHALLOW TRUE
)
```

Then link only the modules needed (Phase 1 needs just the foundation; more modules added in Phase 2):

```cmake
target_link_libraries(Dukko
    PRIVATE
    chowdsp::chowdsp_plugin_state   # Added Phase 2 — listed here so the dep is present
    # Add more chowdsp modules in later phases
)
```

**Note:** For Phase 1 (empty plugin shell), you may link `chowdsp::chowdsp_utils` meta-target or just add the CPM block without linking any modules yet — the goal is to lock the dep and LICENSES.md entry. Linking `chowdsp_plugin_state` in Phase 1 is harmless and avoids a CMake change in Phase 2.

---

## GitHub Actions Workflow Plan

### Runner

Use `macos-14` (Apple Silicon M1, GitHub-hosted). [CITED: CLAUDE.md §"CI / Packaging — macos-14 is the published Apple Silicon runner"]

`macos-15` is also available if `macos-14` becomes deprecated, but `macos-14` is the stable choice as of 2026-05-03. [ASSUMED: verify runner availability on first push; GitHub may update defaults]

### Triggers (D-13)

```yaml
on:
  push:
    branches: ["**"]
  pull_request:
    branches: ["**"]
```

### Jobs

#### Job 1: Release build + validators

```yaml
jobs:
  build-and-validate:
    name: Build & Validate (Release)
    runs-on: macos-14
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive

      - name: Cache CPM sources
        uses: actions/cache@v4
        with:
          path: ~/Library/Caches/CPM
          key: ${{ runner.os }}-cpm-${{ hashFiles('**/CMakeLists.txt', '**/cmake/*.cmake') }}
          restore-keys: |
            ${{ runner.os }}-cpm-

      - name: Configure CMake (Release)
        run: cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64

      - name: Build
        run: cmake --build build --config Release --parallel

      - name: Verify arm64 architecture (VST3)
        run: lipo -archs build/Dukko_artefacts/Release/VST3/Dukko.vst3/Contents/MacOS/Dukko | grep -w arm64

      - name: Verify arm64 architecture (CLAP)
        run: lipo -archs build/Dukko_artefacts/Release/CLAP/Dukko.clap/Contents/MacOS/Dukko | grep -w arm64

      - name: Download pluginval
        run: |
          curl -LO https://github.com/Tracktion/pluginval/releases/download/v1.0.4/pluginval_macOS.zip
          unzip pluginval_macOS.zip
          chmod +x pluginval.app/Contents/MacOS/pluginval

      - name: Run pluginval (strictness 10)
        run: |
          ./pluginval.app/Contents/MacOS/pluginval \
            --strictness-level 10 \
            --validate-in-process \
            --output-dir ./pluginval-results \
            "build/Dukko_artefacts/Release/VST3/Dukko.vst3"

      - name: Run Steinberg VST3 validator
        # TBD-during-execution: locate exact path to validator binary after first build
        # Typical: build/_deps/juce-src/modules/juce_audio_plugin_client/VST3/sdk/bin/validator
        run: |
          VALIDATOR=$(find build/_deps -name "validator" -type f | head -1)
          "$VALIDATOR" "build/Dukko_artefacts/Release/VST3/Dukko.vst3"

      - name: Download clap-validator
        run: |
          curl -LO https://github.com/free-audio/clap-validator/releases/download/0.3.2/clap-validator-0.3.2-macos-arm64.tar.gz
          # TBD-during-execution: verify actual asset filename for 0.3.2 macOS arm64
          tar xf clap-validator-*.tar.gz
          chmod +x clap-validator

      - name: Run clap-validator
        run: |
          ./clap-validator validate \
            "build/Dukko_artefacts/Release/CLAP/Dukko.clap"

      - name: Upload VST3 artifact
        uses: actions/upload-artifact@v4
        with:
          name: Dukko-VST3
          path: build/Dukko_artefacts/Release/VST3/Dukko.vst3
          retention-days: 30

      - name: Upload CLAP artifact
        uses: actions/upload-artifact@v4
        with:
          name: Dukko-CLAP
          path: build/Dukko_artefacts/Release/CLAP/Dukko.clap
          retention-days: 30
```

#### Job 2: Debug + ASan compile-only (D-11)

```yaml
  build-debug-asan:
    name: Build Debug + ASan (compile-only)
    runs-on: macos-14
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive

      - name: Cache CPM sources
        uses: actions/cache@v4
        with:
          path: ~/Library/Caches/CPM
          key: ${{ runner.os }}-cpm-${{ hashFiles('**/CMakeLists.txt', '**/cmake/*.cmake') }}
          restore-keys: |
            ${{ runner.os }}-cpm-

      - name: Configure CMake (Debug + ASan)
        run: |
          cmake -B build-debug \
            -DCMAKE_BUILD_TYPE=Debug \
            -DCMAKE_OSX_ARCHITECTURES=arm64 \
            -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer"

      - name: Build (Debug + ASan)
        run: cmake --build build-debug --config Debug --parallel
```

**Notes:**
- ASan does not run audio in Phase 1 (no DSP); this job only verifies the sanitized build compiles. Phase 2 adds the actual ASan audio run.
- CPM cache key includes all CMakeLists.txt and cmake/*.cmake files (D-14). Both jobs share the same cache key, so whichever runs first will populate it.

### CPM Source Cache Path

Pamplejuce sets `CPM_SOURCE_CACHE` to `~/Library/Caches/CPM` by convention on macOS. Confirm the actual path used after first CI run. The `actions/cache` path above uses that convention.

### Status Badge (README)

Add to `README.md`:
```markdown
[![Build & Validate](https://github.com/[your-org]/Dukko/actions/workflows/build.yml/badge.svg)](https://github.com/[your-org]/Dukko/actions/workflows/build.yml)
```

Replace `[your-org]` with the actual GitHub username/org.

---

## LICENSES.md Format (D-17, QUAL-05)

Create at repo root as a hand-curated markdown table. Template:

```markdown
# Dukko — Third-Party Licenses

This file tracks all third-party dependencies bundled with or linked into Dukko,
plus CI tools that run against the plugin binary. Re-verify at every dep bump and
at any commercial-release decision.

Last updated: 2026-05-03

## Bundled / Linked Dependencies

| Dep | Version pin | License | Source URL | License text |
|-----|-------------|---------|------------|--------------|
| JUCE | 8.0.12 | JUCE 8 Personal EULA (free commercial use ≤$50k/yr) | https://github.com/juce-framework/JUCE | https://juce.com/legal/juce-8-licence/ |
| clap-juce-extensions | `main` @ `<SHA>` | MIT | https://github.com/free-audio/clap-juce-extensions | https://github.com/free-audio/clap-juce-extensions/blob/main/LICENSE |
| chowdsp_utils | v2.4.0 | BSD-3-Clause | https://github.com/Chowdhury-DSP/chowdsp_utils | https://github.com/Chowdhury-DSP/chowdsp_utils/blob/main/LICENSE |
| CPM.cmake | bundled (see cmake/CPM.cmake) | MIT | https://github.com/cpm-cmake/CPM.cmake | https://github.com/cpm-cmake/CPM.cmake/blob/master/LICENSE |
| CLAP headers | transitive via clap-juce-extensions | MIT | https://github.com/free-audio/clap | https://github.com/free-audio/clap/blob/main/LICENSE |

## CI-Only Tools (not linked into Dukko; license does not affect plugin)

| Tool | Version | License | Note |
|------|---------|---------|------|
| pluginval | 1.0.4 | GPLv3 | Runs against the plugin binary, not linked in. GPLv3 does not taint Dukko. |
| clap-validator | 0.3.2 | TBD-during-execution: verify license | Runs against the plugin binary, not linked in. |
| Steinberg VST3 validator | bundled in JUCE 8 SDK fork | Steinberg proprietary | Runs against the plugin binary, not linked in. |
```

**GPLv3 note on pluginval:** pluginval is licensed GPLv3. It is a test-runner that executes the plugin binary in a subprocess. It does not link into the Dukko binary and does not create a derivative work. Dukko's license is unaffected. This note must appear in LICENSES.md to document the reasoning. [CITED: CLAUDE.md §"Validators — pluginval 1.0.4"]

---

## Validation Architecture (Nyquist — Phase 1)

### Test Framework

| Property | Value |
|----------|-------|
| Framework | No unit test framework in Phase 1 (pure build scaffold) |
| Config file | None — validation is CI-based (pluginval, clap-validator, Steinberg) |
| Quick run command | `cmake --build build --config Release && ./pluginval.app/Contents/MacOS/pluginval --strictness-level 10 "build/.../Dukko.vst3"` (local) |
| Full suite command | Push to GitHub → `macos-14` CI job must be green |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| BUILD-01 | CMake build produces VST3 on arm64 | Smoke | `cmake -B build && cmake --build build` exits 0 | ❌ Wave 0 (CI workflow needed) |
| BUILD-02 | Plugin binary is arm64 | Automated | `lipo -archs Dukko.vst3/Contents/MacOS/Dukko \| grep -w arm64` | ❌ Wave 0 |
| BUILD-03 | CLAP bundle produced | Smoke | `test -d build/.../Dukko.clap` | ❌ Wave 0 |
| BUILD-04 | CI job runs on every push | Manual-only (check Actions tab) | N/A | ❌ Wave 0 (.github/workflows/build.yml) |
| BUILD-05 | No Pamplejuce/KickstartClone strings | Automated | `git grep -i 'kickstartclone\|pamplejuce' -- ':!.planning/' ':!CLAUDE.md'` returns empty | ❌ Wave 0 |
| QUAL-01 | pluginval strict-10 green | Automated | pluginval CLI in CI (exit code 0) | ❌ Wave 0 |
| QUAL-05 | LICENSES.md exists with all deps | Automated | `test -f LICENSES.md` + dep-enumeration script (see SC4 below) | ❌ Wave 0 |

### Wave 0 Gaps

All test infrastructure is new (greenfield phase):
- [ ] `.github/workflows/build.yml` — Release build + validator job + Debug+ASan job
- [ ] `LICENSES.md` — dep tracking file
- [ ] `VERSION` file updated to `0.1.0`
- [ ] pluginval download in CI (no framework install — binary download)
- [ ] clap-validator download in CI

---

## Pitfalls & Landmines (Phase 1 specific)

### Pitfall 1: Plugin identifier baked into binaries — NEVER change post-Phase 1

**What goes wrong:** `PLUGIN_MANUFACTURER_CODE`, `PLUGIN_CODE`, and `BUNDLE_ID` are embedded in the binary and VST3 XML manifest. Hosts (Bitwig, Logic, Reaper) use these as the canonical plugin identity for saving project state. Changing them after a plugin has been loaded in any project makes the project unable to find the plugin — Bitwig will report "plugin missing" on every project that ever used the old ID.

**Why it happens:** Developers rename during polish or after a rebrand, not realizing the host already persisted the old IDs.

**How to avoid:** D-01..D-04 are LOCKED. Set them once in Phase 1, never change them. This research documents them; the planner must put them in `CMakeLists.txt` with a comment `# PERMANENT — DO NOT CHANGE — see D-01..D-04 in CONTEXT.md`.

**Warning signs:** pluginval/Steinberg validator errors about mismatched component IDs; Bitwig "plugin missing" errors.

### Pitfall 2: Incomplete rename — template strings survive in unexpected places

**What goes wrong:** Renaming `PROJECT_NAME` and `PRODUCT_NAME` in CMakeLists.txt is not enough. The CLAP manifest, Info.plist template, CI workflow job names, and README may still say "Pamplejuce" or "Demo". The plugin loads fine but the host browser shows wrong names.

**How to avoid:** Run `git grep -r -i 'pamplejuce\|kickstartclone' .` immediately after the initial rename commit. Fix every hit outside `.planning/` and `CLAUDE.md`.

### Pitfall 3: pluginval strict-10 failing on the unmodified Pamplejuce shell

**What goes wrong:** Even an empty Pamplejuce shell (no audio code) can fail pluginval strictness 10 if the wrapped processor mishandles default state (e.g., `getStateInformation` returns an empty block that `setStateInformation` doesn't handle gracefully).

**How to avoid:** Run pluginval locally at strict-5 first, then strict-10. If strict-10 fails on the unmodified template, file a GitHub issue against Pamplejuce and temporarily gate CI at strict-5 with an explicit TODO comment to fix before Phase 2 (where state handling is added anyway).

**Warning signs:** pluginval output includes "state round-trip" or "setState" in the failure reason.

### Pitfall 4: macOS ad-hoc signing required for Bitwig to load on Apple Silicon

**What goes wrong:** On Apple Silicon, macOS Gatekeeper requires binaries to be signed — even with an ad-hoc signature (not a Developer ID). An unsigned `.vst3` or `.clap` bundle will be blocked by Gatekeeper and Bitwig will not load it.

**How to avoid:** JUCE's `COPY_PLUGIN_AFTER_BUILD` path typically runs `codesign --sign -` (ad-hoc) automatically on the installed bundle. **TBD-during-execution:** Verify Pamplejuce's install step runs `codesign --sign -` by checking the build log on first local build. If not, add a post-build CMake command:
```cmake
add_custom_command(TARGET Dukko POST_BUILD
    COMMAND codesign --sign - --force "$<TARGET_BUNDLE_DIR:Dukko>"
)
```

### Pitfall 5: CLAP artifact path differs from VST3 path in build tree

**What goes wrong:** The CI workflow references build artifact paths. JUCE + clap-juce-extensions puts the CLAP bundle at a different path than the VST3 bundle. Getting the path wrong causes the upload-artifact step to silently upload nothing.

**How to avoid:** After first build, run `find build -name "*.clap" -o -name "*.vst3"` and record the exact paths. **TBD-during-execution:** Confirm actual paths and update the workflow YAML accordingly. Typical pattern: `build/Dukko_artefacts/Release/VST3/Dukko.vst3` and `build/Dukko_artefacts/Release/CLAP/Dukko.clap`.

### Pitfall 6: CPM cache key too narrow causes frequent invalidation

**What goes wrong:** Keying the CPM cache only on `CMakeLists.txt` (root file) means changes to any `cmake/*.cmake` file invalidate the cache unnecessarily.

**How to avoid:** Key on `${{ hashFiles('**/CMakeLists.txt', '**/cmake/*.cmake') }}` — all CMakeLists and cmake module files across the repo. This is the recommended pattern per D-14. [ASSUMED: based on common CI caching patterns; verify CPM_SOURCE_CACHE default path on first CI run]

### Pitfall 7: C++23 vs C++20 — Pamplejuce ships C++23

**What goes wrong:** CLAUDE.md recommends C++20 but Pamplejuce's default CMakeLists sets `CMAKE_CXX_STANDARD 23`. This is not a problem — C++23 is a strict superset of C++20 and Apple Clang 15+ supports it. However, if you explicitly set `CMAKE_CXX_STANDARD 20` to match CLAUDE.md, verify this overrides correctly (CMake allows both directions).

**How to avoid:** Leave Pamplejuce's C++23 default in place. C++23 ⊇ C++20; all C++20 features still available; no risk to future Windows/Linux portability with standard toolchains from 2024+.

---

## Validation Checklist (Success Criteria → Concrete Checks)

### SC1: CMake build produces arm64 VST3+CLAP that load in Bitwig

- Automated: `lipo -archs Dukko.vst3/Contents/MacOS/Dukko | grep -w arm64` exits 0
- Automated: `lipo -archs Dukko.clap/Contents/MacOS/Dukko | grep -w arm64` exits 0
- Manual gap: No automated Bitwig load test in Phase 1. Developer must drag-load Dukko in Bitwig, verify it appears in the plugin browser as "Dukko" by "Dukko Audio", and confirm both VST3 and CLAP formats appear.

### SC2: No KickstartClone / Pamplejuce leftovers; everything reads "Dukko"

- Automated: `git grep -i 'kickstartclone\|pamplejuce' -- ':!.planning/' ':!CLAUDE.md' ':!LICENSES.md'` returns empty
- Automated (bundle contents): after build, check that `Dukko.vst3/Contents/Info.plist` contains:
  - `CFBundleIdentifier` = `com.dukkoaudio.dukko`
  - `CFBundleName` = `Dukko`
  - `JUCE_MANUFACTURER_CODE` = `Dukk`
  - `JUCE_PLUGIN_CODE` = `Dkk1`
  - Use: `plutil -p "build/Dukko_artefacts/Release/VST3/Dukko.vst3/Contents/Info.plist" | grep -E 'CFBundle|JUCE'`
- Automated (CLAP manifest): `cat Dukko.clap/Contents/Resources/clap-manifest.json | python3 -c "import sys,json; d=json.load(sys.stdin); assert d['id']=='com.dukkoaudio.dukko'"`

### SC3: Every push triggers macos-14 job; pluginval strict-10 green

- Automated: GitHub Actions job exit codes must all be 0
- pluginval exit code 0 = all tests passed at the specified strictness level
- clap-validator exit code 0 = CLAP spec validation passed
- Steinberg validator exit code 0 = VST3 spec validation passed

### SC4: LICENSES.md complete

- Automated: After build, enumerate CPM-resolved deps by listing `build/_deps/` directories; assert each dep name appears in `LICENSES.md`:
  ```bash
  for dep_dir in build/_deps/*/; do
    dep_name=$(basename "$dep_dir" | sed 's/-src$//')
    grep -qi "$dep_name" LICENSES.md || echo "MISSING in LICENSES.md: $dep_name"
  done
  ```
  A zero-output run means LICENSES.md is complete. Add this as a CI step (can be part of the Release job).

---

## Out-of-Scope Reminders (do NOT include in Phase 1)

- Audio-thread allocation guards / ASan audio runs — Phase 2
- Parameter design / state recall format — Phase 2
- Tempo sync / PPQ-derived phase — Phase 3
- Curve editor — Phase 4
- Codesigning, notarization, installer — deferred until commercial release decision
- Windows runner in CI — v2
- AU / AAX / Standalone formats — excluded from PROJECT.md scope entirely
- `cancel-in-progress` concurrency setting — micro-optimization, any future phase

---

## Open TBDs (for executor)

1. **TBD-during-execution:** After first `cmake` configure run, locate the Steinberg `validator` binary exact path in `build/_deps/` and record it for the CI workflow.
2. **TBD-during-execution:** Record `clap-juce-extensions` `main` HEAD SHA at clone time; substitute `<SHA-TO-FILL-AT-CLONE-TIME>` in the CPM block.
3. **TBD-during-execution:** Verify whether Pamplejuce fetches JUCE via CPM or git submodule on current `main` branch; use the correct mechanism to pin to `8.0.12`.
4. **TBD-during-execution:** Confirm `COPY_PLUGIN_AFTER_BUILD` also copies the CLAP bundle, not just VST3. Check build log for the install line.
5. **TBD-during-execution:** Verify that Pamplejuce's `COPY_PLUGIN_AFTER_BUILD` path includes `codesign --sign -` on the installed bundle (required for Gatekeeper on Apple Silicon). If not, add the post-build CMake command.
6. **TBD-during-execution:** Verify actual VST3 and CLAP bundle output paths in the build tree (`find build -name "*.clap" -o -name "*.vst3"`) and update CI workflow YAML artifact paths.
7. **TBD-during-execution:** Confirm `clap-validator` 0.3.2 macOS arm64 release asset filename and download URL for CI workflow.
8. **TBD-during-execution:** Verify clap-validator license (confirm MIT-compatible for LICENSES.md CI-tool section).
9. **TBD-during-execution:** Read Pamplejuce `.gitignore` on clone and add any missing entries (build dirs, .DS_Store, xcodeproj, IDE caches).
10. **TBD-during-execution:** Update README badge URL with the actual GitHub `[your-org]` value once repo is created.

---

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | Pamplejuce bundles CPM.cmake in `cmake/CPM.cmake` | Dep Pins | CPM may be fetched differently — verify path on clone |
| A2 | CPM_SOURCE_CACHE defaults to `~/Library/Caches/CPM` on macOS | CI Workflow | Wrong cache path → no cache hit, slower CI but no failure |
| A3 | Pamplejuce ships `.gitignore` covering standard build dirs | CMake / Build Config | If absent, binary artifacts may be accidentally committed |
| A4 | `COPY_PLUGIN_AFTER_BUILD` covers both VST3 and CLAP in Pamplejuce | Pamplejuce Rename Plan | CLAP may not auto-install — add manual install step if needed |
| A5 | clap-validator 0.3.2 is MIT-compatible license | LICENSES.md | If incompatible license, update CI-tools note accordingly |
| A6 | C++23 is fully supported by Apple Clang bundled with Xcode 15+ on arm64 | Build Config | If C++23 causes build errors, fall back to C++20 (no API changes for Phase 1) |
| A7 | JUCE 8.0.12 is the latest stable 8.x tag (released Dec 16 2024) | Dep Pins | A newer 8.0.x patch may have been released; check releases page on clone day |

---

## Standard Stack

### Core (from CLAUDE.md — all HIGH confidence, do not re-research)

| Library | Version | Purpose | License |
|---------|---------|---------|---------|
| JUCE | 8.0.12 | Plugin framework, VST3/CLAP wrapping, GUI, CMake integration | JUCE 8 Personal EULA |
| CMake | ≥3.25 | Build system | OSI-approved (bundled in toolchain) |
| CPM.cmake | bundled | Dep fetch wrapper over FetchContent | MIT |
| clap-juce-extensions | main @ SHA | CLAP format wrapper | MIT |
| chowdsp_utils | v2.4.0 | Parameter helpers, plugin state (Phase 2+), preset management (Phase 4+) | BSD-3-Clause |
| Apple Clang / Xcode 15+ | system | macOS arm64 toolchain | Apple proprietary (free via Xcode) |

### CI Tools (not linked, do not appear in Dukko binary)

| Tool | Version | Purpose |
|------|---------|---------|
| pluginval | 1.0.4 | Cross-platform plugin sanity validation |
| clap-validator | 0.3.2 | CLAP spec compliance validation |
| Steinberg VST3 validator | bundled in JUCE | VST3 spec compliance validation |

---

## Sources

### Primary (HIGH confidence — verified this session)
- `raw.githubusercontent.com/sudara/pamplejuce/main/CMakeLists.txt` — exact variable names, defaults, COPY_PLUGIN_AFTER_BUILD location, cmake_minimum_required
- `github.com/juce-framework/JUCE/releases` — JUCE 8.0.12 confirmed as latest stable 8.0.x tag (Dec 16 2024)
- `github.com/free-audio/clap-juce-extensions` — no release tags exist; MIT license; `clap_juce_extensions_plugin()` macro signature confirmed
- `github.com/Chowdhury-DSP/chowdsp_utils` — v2.4.0 latest release (Nov 29 2025); BSD-3-Clause for plugin_state modules
- `github.com/Tracktion/pluginval/releases` — 1.0.4 confirmed latest
- `github.com/free-audio/clap-validator/releases` — 0.3.2 latest (Mar 25 2026)

### Secondary (CITED — from project documents)
- `CLAUDE.md` §"Technology Stack" — all stack rationale, alternatives, version compatibility table (all HIGH confidence, previously verified)
- `.planning/phases/01-build-foundation-ci/01-CONTEXT.md` — all locked decisions D-01..D-17
- `.planning/REQUIREMENTS.md` — phase requirement IDs and descriptions
- `.planning/ROADMAP.md` — phase goal and success criteria

---

## RESEARCH COMPLETE
