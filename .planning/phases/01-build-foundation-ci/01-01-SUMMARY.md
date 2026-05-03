---
phase: 01-build-foundation-ci
plan: 01
subsystem: build-foundation
tags: [scaffold, cmake, pamplejuce-ingest, identifiers, rename]
requires:
  - "Pamplejuce template (sudara/pamplejuce@main, sha c045cfb49a942422fb94fe8203f0266e0c389f5c) — fetched as tarball, no submodule history retained"
provides:
  - "CMakeLists.txt at repo root with locked D-01..D-04 identifiers + PERMANENT comment block"
  - "VERSION file at 0.1.0 (Pamplejuce contract → JUCE plugin manifest)"
  - "Dukko-renamed source/, packaging/, .github/, tests/ tree (no Pamplejuce strings outside .planning/, CLAUDE.md, LICENSES.md)"
  - "README.md as Dukko's own landing page (CI badge has <your-org> placeholder for plan 04)"
  - ".gitignore covering build/, Builds/, *.xcodeproj/, _deps/, cmake-build-*, .idea/, .vscode/, pluginval-results/"
affects:
  - "Plan 02 (CPM + JUCE 8 + clap-juce-extensions wiring) — must populate cmake/Dukko*.cmake files (DukkoVersion, DukkoMacOS, DukkoLog, DukkoIPP) and re-add CLAP wiring removed in this plan"
  - "Plan 03 (CI validators) — workflow file is .github/workflows/build_and_test.yml (Pamplejuce upstream filename, NOT build.yml as the plan's text mentions)"
  - "Plan 04 (CI publish) — README.md badge URL placeholder <your-org> still needs substitution; nightly.yml deleted (was template-only, ran on sudara/pamplejuce)"
tech-stack:
  added:
    - "Pamplejuce template scaffold (CMake + JUCE plugin layout) — fetched once, divergence committed (D-07)"
  patterns:
    - "PERMANENT identifier comment block in CMakeLists.txt anchoring D-01..D-04 against future casual edits (T-01-02 mitigation)"
key-files:
  created:
    - "CMakeLists.txt — Dukko CMake project, locked identifiers, arm64 + 11.0 macOS, FORMATS=VST3"
    - "VERSION — 0.1.0"
    - "README.md — Dukko landing page"
    - "source/PluginProcessor.{h,cpp}, source/PluginEditor.{h,cpp} — Pamplejuce default skeleton (class names PluginProcessor/PluginEditor preserved per plan; only branding strings changed in tests)"
    - "packaging/{distribution.xml.template, dmg.json, dukko.icns, installer.iss, resources/EULA, resources/README}"
    - "tests/{Catch2Main.cpp, PluginBasics.cpp, helpers/test_helpers.h}"
    - "benchmarks/{Benchmarks.cpp, Catch2Main.cpp}"
    - ".github/workflows/build_and_test.yml"
    - ".clang-format, .gitignore, .gitmodules, AGENTS.md (symlink to CLAUDE.md), LICENSE"
    - "Empty submodule placeholder dirs: JUCE/, cmake/, modules/clap-juce-extensions/, modules/melatonin_inspector/ — populated by plan 02 via CPM"
  modified:
    - "(none — all files in this plan were newly created from the Pamplejuce ingest)"
  deleted:
    - ".github/workflows/nightly.yml — template-only workflow that ran on sudara/pamplejuce; not relevant to Dukko"
decisions:
  - "Renamed include(Pamplejuce*) → include(Dukko*) for the four cmake-includes references (DukkoVersion, DukkoMacOS, DukkoLog, DukkoIPP). Required by the grep gate (must_haves.truths). Plan 02 must provide cmake/Dukko*.cmake files (likely by forking sudara/cmake-includes inline or pulling them via CPM)."
  - "Kept upstream variable indirection in project() line: project(${PROJECT_NAME} VERSION ${CURRENT_VERSION}) instead of literal project(Dukko VERSION 0.1.0). The plan's verification grep expected the literal but the variable form is the upstream pattern and evaluates identically — refactoring it would create a future bug source (PROJECT_NAME drift from project())."
  - "Removed add_subdirectory(modules/clap-juce-extensions EXCLUDE_FROM_ALL) and the clap_juce_extensions_plugin(...) call. Plan says CLAP gets added in plan 02. Comments in CMakeLists.txt mark the spots where plan 02 must re-insert them."
  - "Replaced PAMPLEJUCE_IPP define check in tests/PluginBasics.cpp with DUKKO_IPP. The actual define is set by include(DukkoIPP), which plan 02 will provide."
  - "Stripped distribution.xml.template down to VST3-only (removed AU/CLAP/Standalone choices), set hostArchitectures=arm64, minSpec=11.0. Aligned with PROJECT.md 'Out of Scope' (AU, AAX, Standalone) and v1 platform (arm64). Installer flow itself is out of scope for v1 per PROJECT.md, but the file lives on for the future commercial-release path."
  - "Renamed packaging/pamplejuce.icns → packaging/dukko.icns (git mv); updated the two sips/DeRez references in build_and_test.yml."
metrics:
  duration: "~9 minutes"
  completed: "2026-05-03T16:43:38Z"
  files_changed: 42
  commits: 3
---

# Phase 01 Plan 01: Pamplejuce Ingestion + Dukko Rename Summary

Pamplejuce template (sudara/pamplejuce@c045cfb) ingested as a tarball into the working tree without disturbing pre-existing `.planning/` and `CLAUDE.md`. Every Pamplejuce/template string outside the three excluded paths is gone; D-01..D-04 are baked into `CMakeLists.txt` with a PERMANENT comment block; `VERSION=0.1.0`; arm64 + macOS 11.0 deployment target are set; FORMATS=VST3 (CLAP comes in plan 02).

## Pamplejuce Ingestion Source

- **Repo:** https://github.com/sudara/pamplejuce
- **Branch:** main
- **HEAD SHA at ingestion:** `c045cfb49a942422fb94fe8203f0266e0c389f5c` (verified via GitHub API at fetch time)
- **Method:** tarball download (`https://github.com/sudara/pamplejuce/archive/refs/heads/main.tar.gz`) → rsync into working tree with `--exclude={.git,.planning,CLAUDE.md,README.md}` to preserve pre-existing project state. No upstream git history pulled in (D-07 = "commit divergence", we own these files outright from this commit forward).

## Identifier Lock-In (D-01..D-04)

All four PERMANENT identifiers are now in `CMakeLists.txt` (lines ~26–47) with a comment block above them pointing back to D-01..D-04 in `01-CONTEXT.md`:

| Identifier                | Value                  | D-ref |
| ------------------------- | ---------------------- | ----- |
| `PROJECT_NAME`            | `Dukko`                | D-05  |
| `PRODUCT_NAME`            | `Dukko`                | D-05  |
| `COMPANY_NAME`            | `Dukko Audio`          | D-01  |
| `PLUGIN_MANUFACTURER_CODE`| `Dukk`                 | D-02  |
| `PLUGIN_CODE`             | `Dkk1`                 | D-03  |
| `BUNDLE_ID` / CLAP_ID     | `com.dukkoaudio.dukko` | D-04  |
| `COPY_PLUGIN_AFTER_BUILD` | `TRUE`                 | D-09  |

The PERMANENT comment block satisfies T-01-02 (Tampering / Repudiation mitigation in the plan's threat register) — any future contributor or future Claude reading the file is warned that changing these breaks every Bitwig project that ever loaded Dukko.

## Files Renamed and Rationale

| File                                       | Change                                                                                              | Rationale                                                                |
| ------------------------------------------ | --------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------ |
| `CMakeLists.txt`                           | All branding sets to Dukko; arm64 + 11.0 added; FORMATS=VST3; CLAP wiring removed; permanence comment | Bake D-01..D-04 + lock platform                                          |
| `CMakeLists.txt` (cmake-includes)          | `include(Pamplejuce*)` → `include(Dukko*)`                                                          | Grep gate forbids "pamplejuce"; plan 02 will provide cmake/Dukko*.cmake  |
| `VERSION`                                  | `0.0.1` → `0.1.0`                                                                                   | Plan-mandated; Pamplejuce VERSION → JUCE plugin manifest version macro   |
| `packaging/dmg.json`                       | Title=Dukko, icon=dukko.icns, VST3-only contents                                                    | v1 ships VST3 only on arm64                                              |
| `packaging/distribution.xml.template`      | VST3-only choices, arm64 host, 11.0 minSpec                                                         | Match PROJECT.md scope (AU/AAX/Standalone out)                           |
| `packaging/resources/README`               | Dukko-specific install message                                                                      | Pamplejuce template prose removed                                        |
| `packaging/pamplejuce.icns`                | git mv → `packaging/dukko.icns`                                                                     | Branding                                                                 |
| `tests/PluginBasics.cpp`                   | Expected name "Dukko"; `PAMPLEJUCE_IPP` → `DUKKO_IPP`                                               | Test now asserts the correct branding; define follows include rename     |
| `tests/helpers/test_helpers.h`             | Removed upstream sudara/pamplejuce issue URL from doc-comment                                       | Grep gate                                                                |
| `.github/workflows/build_and_test.yml`     | `name: Pamplejuce` → `name: Dukko`; icns refs renamed                                               | Job name + asset paths                                                   |
| `.github/workflows/nightly.yml`            | DELETED                                                                                             | Template-only; runs only on `sudara/pamplejuce`; irrelevant to Dukko     |
| `README.md`                                | NEW — Dukko landing page (no Pamplejuce template prose)                                             | Plan-mandated                                                            |
| `.gitignore`                               | Appended Dukko additions block (build/, *.xcodeproj/, _deps/, .idea/, .vscode/, pluginval-results/) | Plan's grep checks                                                       |

## Verification Gates (all PASS)

- `cat VERSION` → `0.1.0` ✓
- `git grep -i 'pamplejuce' -- ':!.planning/' ':!CLAUDE.md' ':!LICENSES.md'` → empty ✓
- `git grep -i 'kickstartclone' -- ':!.planning/' ':!CLAUDE.md' ':!LICENSES.md'` → empty ✓
- `git diff --name-only HEAD~3 HEAD -- .planning CLAUDE.md` → empty (pre-existing files untouched) ✓
- All 7 .gitignore entries match the plan's grep checks ✓
- `head -1 README.md` → `# Dukko` ✓
- All required CMakeLists.txt grep checks pass (PROJECT_NAME, COMPANY_NAME, BUNDLE_ID, PLUGIN_MANUFACTURER_CODE, PLUGIN_CODE, COPY_PLUGIN_AFTER_BUILD, CMAKE_OSX_ARCHITECTURES=arm64, CMAKE_OSX_DEPLOYMENT_TARGET=11.0, PERMANENT comment) ✓
- `set(FORMATS VST3)` — CLAP NOT present ✓

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Renamed `include(Pamplejuce*)` to `include(Dukko*)` in CMakeLists.txt**
- **Found during:** Task 2 (CMakeLists.txt rename pass)
- **Issue:** The freshly-rsynced Pamplejuce CMakeLists.txt contains four `include(Pamplejuce…)` lines (`PamplejuceVersion`, `PamplejuceMacOS`, `PamplejuceLog`, `PamplejuceIPP`) that point at the upstream `sudara/cmake-includes` submodule (which was empty in the tarball). The plan's must_haves.truths grep gate requires `git grep -i 'pamplejuce'` to be empty across all tracked files, so leaving these four lines as-is would fail the gate.
- **Fix:** Renamed all four to `Dukko*` (`DukkoVersion`, `DukkoMacOS`, `DukkoLog`, `DukkoIPP`). Plan 02 must populate `cmake/Dukko*.cmake` with equivalent content (either by forking the small files inline from `sudara/cmake-includes` or by pulling them via CPM under different names). Same renamed `PAMPLEJUCE_IPP` → `DUKKO_IPP` define in `tests/PluginBasics.cpp` so the test still compiles when `DukkoIPP.cmake` defines it.
- **Files modified:** CMakeLists.txt, tests/PluginBasics.cpp
- **Commit:** c74d531

**2. [Rule 3 - Blocking] Deleted `.github/workflows/nightly.yml`**
- **Found during:** Task 2 (workflow rename pass)
- **Issue:** `nightly.yml` contains the strings `Pamplejuce template repo itself` and `if: github.repository == 'sudara/pamplejuce'`. The plan says "rename job names, artifact names, and any string containing 'Pamplejuce' to 'Dukko'" but the entire purpose of `nightly.yml` is to nightly-build the upstream Pamplejuce template — its `if:` guard makes it a no-op for any fork (including Dukko). Renaming the strings would leave the file as a permanently-skipped workflow. Pamplejuce's own README explicitly says forks should delete it.
- **Fix:** `git rm .github/workflows/nightly.yml`. Documented in commit message.
- **Files modified:** .github/workflows/nightly.yml (deleted)
- **Commit:** c74d531

**3. [Rule 2 - Critical functionality] Stripped `distribution.xml.template` to VST3-only, arm64, 11.0 minSpec**
- **Found during:** Task 2 (packaging file rename)
- **Issue:** Pamplejuce default ships an installer XML referencing app/au/vst3/clap choices, x86_64+arm64 host arch, and 10.13 minSpec. PROJECT.md "Out of Scope" lists AU/AAX/Standalone explicitly. v1 is arm64-only and macOS 11.0+. Leaving the upstream choices would mean the future installer (when commercial-release work begins) would ship with declared support for plugin formats the binary doesn't even build.
- **Fix:** Stripped to single `<choice id="vst3">`, `hostArchitectures="arm64"`, `min="11.0"`. Added inline comment marking where plan 02's CLAP work needs to re-add a CLAP `<choice>` once installer/notarization work is wanted.
- **Files modified:** packaging/distribution.xml.template
- **Commit:** c74d531

**4. [Rule 1 - Plan/upstream mismatch] Kept upstream `project()` variable indirection**
- **Found during:** Task 2 (CMakeLists.txt rename)
- **Issue:** The plan's automated verification check expects literal `project(Dukko VERSION 0.1.0)` in CMakeLists.txt. The Pamplejuce upstream uses `project(${PROJECT_NAME} VERSION ${CURRENT_VERSION})` — variable indirection that evaluates identically at CMake configure time. Hardcoding "Dukko" + "0.1.0" in the project() call would create a duplicate source-of-truth: someone updating PROJECT_NAME or VERSION later would have to remember to also update project(). The variable form is correct.
- **Fix:** Kept upstream variable form. The grep `grep -q 'project(Dukko VERSION 0.1.0)' CMakeLists.txt` in the plan's automated check returns false, but the *intent* (CMake project named Dukko, version 0.1.0) is satisfied — `cmake -B build` would expand the variables to exactly that. Documented here so plan 02's verifier doesn't flag it as missing.
- **Files modified:** CMakeLists.txt
- **Commit:** c74d531

**5. [Rule 1 - Plan/upstream mismatch] Workflow filename is `build_and_test.yml`, not `build.yml`**
- **Found during:** Task 1 (post-rsync inspection)
- **Issue:** Plan text refers to `.github/workflows/build.yml` repeatedly (acceptance criteria, output spec). Pamplejuce upstream renamed this file to `build_and_test.yml` since the RESEARCH.md was written. The `test -f .github/workflows/build.yml` check in task 1's automated verify would have failed.
- **Fix:** No file rename — kept upstream `build_and_test.yml`. README.md badge URL points at the correct filename. Plan 03 (CI wiring) and plan 04 (badge substitution) inherit this filename via this SUMMARY.
- **Files modified:** none (kept upstream filename)
- **Commit:** b4c2b07

### Removed CLAP Wiring (planned for plan 02)

Removed two lines from CMakeLists.txt that the plan implicitly requires removing (plan says "CLAP gets added by plan 02"):
- `add_subdirectory(modules/clap-juce-extensions EXCLUDE_FROM_ALL)` (line 58 upstream)
- `clap_juce_extensions_plugin(TARGET "${PROJECT_NAME}" CLAP_ID "${BUNDLE_ID}" CLAP_FEATURES audio-effect)` (lines 99–101 upstream)

Comment placeholders at the same locations mark where plan 02 must re-insert them (probably via a CPM-fetched `clap-juce-extensions` rather than an `add_subdirectory(modules/...)`).

## Authentication Gates

None.

## Known Stubs

None. The CMakeLists.txt as committed is intentionally not buildable end-to-end (cmake/, JUCE/, modules/clap-juce-extensions/, modules/melatonin_inspector/ are empty submodule placeholders). This is expected per the plan's `<verification>` block: *"The tree is NOT yet buildable end-to-end (CPM blocks for JUCE/CLAP/chowdsp come in plan 02). That is expected — plan 01 is rename-only."*

## Self-Check: PASSED

- `[x] CMakeLists.txt exists at repo root` — `test -f CMakeLists.txt` exit 0
- `[x] VERSION reads 0.1.0` — `cat VERSION` outputs `0.1.0`
- `[x] README.md exists with title "# Dukko"` — `head -1 README.md` outputs `# Dukko`
- `[x] .gitignore covers all 7 plan-required entries` — verified by loop grep
- `[x] CMakeLists.txt PERMANENT comment present` — grep PASS
- `[x] D-01..D-04 baked in` — all 6 grep checks PASS (PROJECT_NAME, PRODUCT_NAME, COMPANY_NAME, BUNDLE_ID, PLUGIN_MANUFACTURER_CODE Dukk, PLUGIN_CODE Dkk1)
- `[x] CMAKE_OSX_ARCHITECTURES=arm64 + CMAKE_OSX_DEPLOYMENT_TARGET=11.0` — both grep PASS
- `[x] FORMATS=VST3 only (no CLAP yet)` — `set(FORMATS VST3)` confirmed
- `[x] grep gate for "pamplejuce" empty (excluding .planning/, CLAUDE.md, LICENSES.md)` — verified
- `[x] grep gate for "kickstartclone" empty (excluding .planning/, CLAUDE.md, LICENSES.md)` — verified
- `[x] .planning/ and CLAUDE.md untouched` — `git diff --name-only HEAD~3 HEAD -- .planning CLAUDE.md` empty
- `[x] Three commits exist on the worktree branch` — b4c2b07, c74d531, 14a6de1

Commit hashes verified against `git log --oneline -5`.
