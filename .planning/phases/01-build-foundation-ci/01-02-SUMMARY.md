---
phase: 01-build-foundation-ci
plan: 02
subsystem: build-foundation
tags: [cmake, cpm, juce, clap, chowdsp, licenses, dep-pinning]
requires:
  - "Plan 01-01 outputs (PERMANENT D-01..D-04 block in CMakeLists.txt; cmake/ placeholder dir empty + JUCE/ + modules/* empty placeholders; .gitmodules listing four submodules; tests/PluginBasics.cpp guarding on DUKKO_IPP)"
  - "Network access to GitHub during cmake configure (CPM clones JUCE 8.0.12, clap-juce-extensions @ SHA, chowdsp_utils v2.4.0, melatonin_inspector @ SHA, Catch2 v3.8.1)"
  - "CMake ≥ 3.25 installed locally for the verify-gate run (the developer machine had no cmake; installed via `pip3 install --user cmake` → cmake 4.3.2 in ~/Library/Python/3.9/bin)"
provides:
  - "CMakeLists.txt with three runtime CPM deps (JUCE 8.0.12, clap-juce-extensions e8de9e8, chowdsp_utils v2.4.0) + a fourth tooling dep (melatonin_inspector @ 9e91e4e) + clap_juce_extensions_plugin(TARGET Dukko ...) call producing a Dukko_CLAP target alongside the JUCE-VST3 target"
  - "Vendored cmake/ helpers (12 files): cmake/CPM.cmake + cmake/Dukko{Version,MacOS,Log,IPP}.cmake (renamed from sudara/cmake-includes Pamplejuce*) + cmake/{JUCEDefaults,SharedCodeDefaults,Assets,XcodePrettify,Tests,Benchmarks,GitHubENV}.cmake (verbatim from sudara/cmake-includes, MIT)"
  - "LICENSES.md at repo root tracking 8 bundled/linked deps + 3 CI-only tools per D-17 format, with the GPLv3-pluginval reasoning paragraph"
  - ".gitmodules emptied (4 stale submodule entries removed) since CPM replaces all submodules"
affects:
  - "Plan 03 (CI workflow + validators) — can now run `cmake -B build && cmake --build build` end-to-end on macos-14; the QUAL-05 enumeration loop (`for d in build/_deps/*/; do grep -qi $name LICENSES.md ...`) is ready to wire as a CI gate (T-02-03 mitigation)"
  - "Plan 04 (CI publish + first push) — README badge URL still has `<your-org>` placeholder; first push will exercise the full CI workflow end-to-end including pluginval, clap-validator, and Steinberg validator"
  - "Phase 2 (DSP scaffold) — chowdsp_utils dep is present; can `target_link_libraries(Dukko PRIVATE chowdsp::chowdsp_plugin_state)` without any CMake-block change (D-15 win)"
tech-stack:
  added:
    - "JUCE 8.0.12 (tag) — released 2024-12-16; the actual plugin framework. Resolved Open TBD #3: upstream uses git-submodule; we picked the CPM path."
    - "clap-juce-extensions @ e8de9e8571626633b8541a54c2406fccc4272767 (main HEAD on 2026-05-03; CLAP wrapper version 1.2.7). Resolved Open TBD #2."
    - "chowdsp_utils v2.4.0 (tag) — fetched, not yet linked"
    - "melatonin_inspector @ 9e91e4e3d6cc41688c8d2108ef7ed33c1a90dcc9 (no release tags exist; pinned to main HEAD)"
    - "CPM.cmake v0.42.0 (downloaded by cmake/CPM.cmake at configure time)"
    - "Catch2 v3.8.1 (transitive: pulled by cmake/Tests.cmake into the Tests + Benchmarks targets)"
  patterns:
    - "GIT_SHALLOW FALSE for any dep pinned to a SHA (clap-juce-extensions, melatonin_inspector); GIT_SHALLOW TRUE for tag-pinned deps (JUCE, chowdsp_utils, Catch2)"
    - "Symlink workaround for melatonin_inspector: CPM extracts to `<name>-src` but JUCE's juce_add_module asserts dir basename == module ID; cmake/CMakeLists.txt creates a configure-time symlink `build/_deps/melatonin_inspector → melatonin_inspector-src`"
    - "Literal D-04 IDs at the clap_juce_extensions_plugin call site (TARGET Dukko, CLAP_ID \"com.dukkoaudio.dukko\") — duplicates the PERMANENT block above but is justified by (a) plan-02 grep gate, (b) future-reader sees canonical CLAP id at the wiring site"
key-files:
  created:
    - "LICENSES.md (repo root)"
    - "cmake/CPM.cmake — bootstraps CPM v0.42.0 download"
    - "cmake/DukkoVersion.cmake — VERSION file → CURRENT_VERSION (renamed PAMPLEJUCE_AUTO_BUMP_PATCH_LEVEL → DUKKO_AUTO_BUMP_PATCH_LEVEL)"
    - "cmake/DukkoMacOS.cmake — Xcode scheme noise toggle only (root CMakeLists owns arm64/11.0)"
    - "cmake/DukkoLog.cmake — build-env logging"
    - "cmake/DukkoIPP.cmake — IPP detection (renamed PAMPLEJUCE_IPP → DUKKO_IPP)"
    - "cmake/JUCEDefaults.cmake — JUCE-friendly CMake defaults (USE_FOLDERS, MSVC runtime, etc.)"
    - "cmake/SharedCodeDefaults.cmake — Release flags + cxx_std_20 lock (changed from upstream's cxx_std_23 per CLAUDE.md)"
    - "cmake/Assets.cmake — assets/ → BinaryData target"
    - "cmake/XcodePrettify.cmake — Xcode source-tree organization"
    - "cmake/Tests.cmake — Catch2 + Tests target (renamed RUN_PAMPLEJUCE_TESTS → RUN_DUKKO_TESTS)"
    - "cmake/Benchmarks.cmake — Catch2 Benchmarks target (same rename)"
    - "cmake/GitHubENV.cmake — writes .env for CI packaging"
  modified:
    - "CMakeLists.txt — replaced add_subdirectory(JUCE) + add_subdirectory(modules/melatonin_inspector) with three CPM blocks (JUCE/clap-juce-extensions/chowdsp_utils) + melatonin_inspector CPM block + symlink workaround + juce_add_module call + clap_juce_extensions_plugin(TARGET Dukko CLAP_ID \"com.dukkoaudio.dukko\" ...) call"
    - ".gitmodules — emptied; explanatory comment notes CPM replaces all four submodules"
  deleted:
    - "(none)"
decisions:
  - "Resolved Open TBD #3 (JUCE fetch mechanism): chose CPM path. Upstream Pamplejuce uses a git submodule, but the rsynced tarball had no JUCE/.git data, and adding a submodule retroactively means a non-trivial `.git/modules/...` setup that's worse for cloning DX than CPM. CPM also gives identical reproducibility via GIT_TAG 8.0.12 + GIT_SHALLOW TRUE."
  - "Pinned clap-juce-extensions to SHA e8de9e8571626633b8541a54c2406fccc4272767 (main HEAD on 2026-05-03). No release tags exist on this repo; bump deliberately on next major JUCE bump or after auditing upstream changes."
  - "Vendored cmake-includes (sudara/cmake-includes, MIT) inline rather than CPM-fetching them. Reason: those are 12 small text files (~50–100 lines each) we own outright per D-07 commit-divergence; CPM-fetching adds a network dep for trivial helper code. Plan 01-01 SUMMARY explicitly handed off `must populate cmake/Dukko*.cmake files` to plan 02 — vendoring delivers on that handoff."
  - "Used the BUNDLE_ID variable inside the PERMANENT comment block but the LITERAL `\"com.dukkoaudio.dukko\"` at the clap_juce_extensions_plugin call site. The plan's grep gate (`grep -q 'CLAP_ID \"com.dukkoaudio.dukko\"' CMakeLists.txt`) requires the literal; using `${BUNDLE_ID}` would have failed. Future-reader cost is one duplicate string; safety upside is a future Bitwig-id-change must touch BOTH locations, which is good (the PERMANENT block warns; the wiring site reinforces)."
  - "Replaced melatonin_inspector add_subdirectory with CPM + symlink workaround. Upstream melatonin_inspector's CMakeLists.txt asserts the directory basename equals the module ID; CPM extracts into `<name>-src`. Workaround: configure-time `file(CREATE_LINK)` creates `build/_deps/melatonin_inspector → melatonin_inspector-src`, then juce_add_module is called on the symlink path. Alternative considered: PATCH the upstream CMakeLists.txt during CPM's PATCH step — rejected because PATCH-step diffs are brittle across upstream bumps."
  - "Lowered cxx_std_23 → cxx_std_20 in SharedCodeDefaults.cmake. Per CLAUDE.md tech-stack lock: \"C++20 ... no DSP cost; C++23 is technically available but adds nothing essential and reduces toolchain margin. Skip.\""
  - "Emptied .gitmodules instead of deleting it. Some tooling expects the file to exist on JUCE-flavored repos; an empty file with an explanatory comment is the least-surprising state."
metrics:
  duration: "~28 minutes (mostly waiting on cmake configure + CPM clones — first run cold-cloned ~700MB across JUCE+chowdsp+clap-juce-extensions+melatonin_inspector+Catch2)"
  completed: "2026-05-03T18:54:00Z"
  files_changed: 15
  commits: 3
---

# Phase 01 Plan 02: CPM Deps + CLAP Wiring + LICENSES.md Summary

Wired JUCE 8.0.12, clap-juce-extensions @ e8de9e8 (main HEAD 2026-05-03), and chowdsp_utils v2.4.0 into CMakeLists.txt as CPM dependencies; added `clap_juce_extensions_plugin(TARGET Dukko CLAP_ID "com.dukkoaudio.dukko" ...)` so the build now produces a `Dukko_CLAP` target alongside the JUCE VST3 target; created LICENSES.md at repo root tracking 8 bundled/linked deps + 3 CI-only tools with the GPLv3-pluginval reasoning paragraph; locally verified `cmake -B build -DCMAKE_BUILD_TYPE=Release` configures cleanly end-to-end.

## TBD-during-execution Resolutions

### TBD #2: clap-juce-extensions SHA

Recorded SHA: **`e8de9e8571626633b8541a54c2406fccc4272767`** (refs/heads/main on 2026-05-03 via `git ls-remote https://github.com/free-audio/clap-juce-extensions.git refs/heads/main`).

Pin lives at `CMakeLists.txt:99` inside the CPM block, with a near-line comment recording the recorded date for the next bump.

### TBD #3: JUCE fetch mechanism (CPM vs submodule)

Inspected the post-plan-01 tree: no live JUCE/ submodule (rsynced tarball had no `.git` data; the directory was an empty placeholder). Picked the **CPM path** — added a CPMAddPackage block pinned to tag `8.0.12`. Reproducibility is identical to a submodule pin; cloning DX is better (no `git submodule update --init --recursive`).

## Open TBDs Still Outstanding (handed forward)

- TBD #5: Steinberg VST3 validator binary path inside `build/_deps/juce-src/...`. Not located in plan 02 because no actual build was attempted (configure-only). Plan 03's CI step uses `find build/_deps -name "validator" -type f` to locate it dynamically — that's still the right approach.
- TBD #8: clap-validator license — RESOLVED in LICENSES.md (MIT, verified 2026-05-03 via GitHub API on https://api.github.com/repos/free-audio/clap-validator).

## CMakeLists.txt — End-State Diff Summary

| Block (line range, post-edit) | Change |
|------------------------------|--------|
| 9–14 | Comment update: cmake/ folder is no longer empty; lists the vendored helpers |
| 70–86 | NEW: 16-line comment block documenting the three runtime dep pins (D-15, D-16) + the LICENSES.md cross-reference |
| 87–92 | NEW: JUCE CPM block (replaces deleted `add_subdirectory(JUCE)`) |
| 94–101 | NEW: clap-juce-extensions CPM block with the recorded SHA (replaces deleted `add_subdirectory(modules/clap-juce-extensions)`) |
| 103–109 | NEW: chowdsp_utils CPM block + Phase 2 link-target reminder |
| 111–122 | NEW: melatonin_inspector CPM block (DOWNLOAD_ONLY) — replaces deleted `add_subdirectory(modules/melatonin_inspector)` |
| 128–135 | NEW: symlink workaround + juce_add_module call for melatonin_inspector |
| 154–163 | NEW: clap_juce_extensions_plugin(TARGET Dukko CLAP_ID "com.dukkoaudio.dukko" CLAP_FEATURES audio-effect mixing) |
| 26–37 | UNCHANGED: PERMANENT D-01..D-04 block from plan 01 (verified intact) |

Full file is 230 lines (was 184). All net-additions are dep-wiring or comments; no semantic change to existing JUCE plugin config.

## LICENSES.md Snapshot at Completion

```
## Bundled / Linked Dependencies
| JUCE                  | 8.0.12      | JUCE 8 Personal EULA |
| clap-juce-extensions  | main @ e8de9e8…   | MIT |
| chowdsp_utils         | v2.4.0      | BSD-3-Clause |
| CPM.cmake             | bundled (downloads v0.42.0)  | MIT |
| CLAP headers          | transitive via clap-juce-ext | MIT |
| melatonin_inspector   | main @ 9e91e4e…   | MIT |
| Catch2                | v3.8.1 (transitive via cmake/Tests.cmake) | BSL-1.0 |
| sudara/cmake-includes | vendored (D-07) | MIT |

## CI-Only Tools
| pluginval        | 1.0.4  | GPLv3 (test-runner subprocess; does not taint Dukko)            |
| clap-validator   | 0.3.2  | MIT (verified 2026-05-03 via GitHub API)                        |
| Steinberg VST3 validator | bundled in JUCE 8 SDK fork | Steinberg proprietary |
```

Plus the GPLv3 reasoning paragraph (verbatim from RESEARCH.md §"GPLv3 note on pluginval").

Future-bump diff path: change a version cell + verify license URL still resolves.

## `cmake -B build` Output (dep-resolution lines)

```
-- CPM: Adding package JUCE@8.0.12 (8.0.12)
-- Configuring juceaide
-- Building juceaide
-- Finished setting up juceaide
-- CPM: Adding package clap-juce-extensions@0 (e8de9e8571626633b8541a54c2406fccc4272767)
-- Building CLAP with CLAP_CXX_STANDARD=17
-- CLAP version: 1.2.7
-- CPM: Adding package chowdsp_utils@2.4.0 (v2.4.0)
-- Adding ChowDSP JUCE modules...
-- CPM: Adding package melatonin_inspector@0 (9e91e4e3d6cc41688c8d2108ef7ed33c1a90dcc9)
-- Populating melatonin_inspector
-- Creating CLAP Dukko_CLAP from Dukko
-- CPM: Adding package Catch2@3.8.1 (v3.8.1)
-- Configuring done (2.4s)
-- Generating done (0.3s)
```

`Dukko_CLAP` target proves CLAP wiring is live. `Adding ChowDSP JUCE modules...` confirms chowdsp resolved (even though we don't link any module yet — D-15 is now satisfied). The `juceaide` build is JUCE 8's auto-generated tool, not a Dukko binary.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Vendored 12 cmake/ helper files (CPM.cmake + Dukko*.cmake + 7 unrenamed helpers)**
- **Found during:** Task 1 first cmake configure attempt
- **Issue:** Plan 01-01 SUMMARY documented that the rsynced Pamplejuce tarball had empty placeholder dirs for cmake/, JUCE/, modules/melatonin_inspector/, modules/clap-juce-extensions/. The CMakeLists.txt at task 1's start has `include(DukkoVersion)`, `include(CPM)`, `include(DukkoMacOS)`, `include(JUCEDefaults)`, `include(DukkoLog)`, `include(SharedCodeDefaults)`, `include(Assets)`, `include(XcodePrettify)`, `include(DukkoIPP)`, `include(Tests)`, `include(Benchmarks)`, `include(GitHubENV)` — twelve include() calls referencing files that don't exist. The plan's verify gate `cmake -B build -DCMAKE_BUILD_TYPE=Release` cannot pass without these files present.
- **Fix:** Vendored all twelve from `sudara/cmake-includes` (MIT). Renamed Pamplejuce* → Dukko* for the four core helpers per plan 01-01 SUMMARY's explicit handoff (`Plan 02 ... must populate cmake/Dukko*.cmake files`). Kept the other seven verbatim (their filenames don't carry branding). Adjusted `SharedCodeDefaults.cmake` from `cxx_std_23` → `cxx_std_20` per CLAUDE.md tech-stack lock. License attribution lives in LICENSES.md (`sudara/cmake-includes (vendored as cmake/Dukko*.cmake + ...)`).
- **Files modified:** cmake/CPM.cmake, cmake/DukkoVersion.cmake, cmake/DukkoMacOS.cmake, cmake/DukkoLog.cmake, cmake/DukkoIPP.cmake, cmake/JUCEDefaults.cmake, cmake/SharedCodeDefaults.cmake, cmake/Assets.cmake, cmake/XcodePrettify.cmake, cmake/Tests.cmake, cmake/Benchmarks.cmake, cmake/GitHubENV.cmake (all created)
- **Commit:** c688c52

**2. [Rule 3 - Blocking] Replaced melatonin_inspector add_subdirectory with CPM + symlink workaround**
- **Found during:** Task 1 second cmake configure attempt
- **Issue:** CMakeLists.txt had `add_subdirectory(modules/melatonin_inspector)` pointing at an empty placeholder dir. The plan's `<files>` list only mentions CMakeLists.txt + LICENSES.md — it doesn't address melatonin_inspector. But the line is in the file, the directory is empty, and the configure step explodes immediately. Could not be left as-is.
- **Fix:** Added a fourth CPM block fetching melatonin_inspector @ 9e91e4e (main HEAD; no release tags). Used `DOWNLOAD_ONLY YES` because the upstream CMakeLists asserts directory basename == module name, but CPM extracts into `<name>-src`. Configure-time symlink (`file(CREATE_LINK)`) creates `build/_deps/melatonin_inspector → melatonin_inspector-src`; juce_add_module is then called on the symlink path. LICENSES.md tracks the dep.
- **Files modified:** CMakeLists.txt (added CPM block + symlink + juce_add_module call), LICENSES.md (added row)
- **Commit:** c688c52

**3. [Rule 2 - Critical functionality] Added Catch2 v3.8.1 to LICENSES.md**
- **Found during:** Task 2 QUAL-05 enumeration check
- **Issue:** cmake/Tests.cmake (vendored as part of deviation #1 above) calls `CPMAddPackage("gh:catchorg/Catch2@3.8.1")` to provide the Tests + Benchmarks targets. That's a transitive dep of Dukko's CMake graph. QUAL-05 says LICENSES.md tracks all deps — Catch2 wasn't in the plan's enumeration but it's now in `build/_deps/`. Leaving it out fails the enumeration check (`for d in build/_deps/*/; do grep -qi $name LICENSES.md ...`).
- **Fix:** Added Catch2 row to LICENSES.md under "Bundled / Linked Dependencies" with a note that it's transitive via cmake/Tests.cmake. License: BSL-1.0 (Boost Software License — commercial-use OK, attribution-style).
- **Files modified:** LICENSES.md
- **Commit:** 1e18d76

**4. [Rule 3 - Blocking] Emptied .gitmodules (4 stale submodule entries removed)**
- **Found during:** Task 1 (not really discovered, but acted on)
- **Issue:** `.gitmodules` had four entries (JUCE, cmake, modules/melatonin_inspector, modules/clap-juce-extensions) all pointing at upstream repos that we're now CPM-fetching instead. Stale entries would confuse `git submodule update` — git would try to init submodules for paths that have no submodule data (`.git/modules/...` doesn't exist). Cleaner to drop them.
- **Fix:** Replaced .gitmodules content with an explanatory comment that Dukko has no git submodules — all former-submodule deps are CPM-fetched now.
- **Files modified:** .gitmodules
- **Commit:** c688c52

**5. [Rule 1 - Plan/upstream mismatch] Pinned melatonin_inspector to a SHA, not a tag**
- **Found during:** Task 1 (first cmake configure attempt failed with `fatal: invalid reference: 1.7.0`)
- **Issue:** Initially guessed melatonin_inspector tag `1.7.0` (because the upstream CMakeLists.txt at HEAD declares `VERSION 1.3.0` and the README mentions 1.x). `git ls-remote --tags` returned NOTHING — the repo has no release tags. Only branches.
- **Fix:** Re-pinned to `9e91e4e3d6cc41688c8d2108ef7ed33c1a90dcc9` (refs/heads/main HEAD on 2026-05-03 via `git ls-remote ... refs/heads/main`). Set `GIT_SHALLOW FALSE` because shallow clone fails for non-tag SHAs.
- **Files modified:** CMakeLists.txt
- **Commit:** c688c52

**6. [Rule 2 - Critical functionality] Re-scrubbed Pamplejuce string from comments after vendoring**
- **Found during:** Post-task-2 grep regression check
- **Issue:** Plan 01-01 SUMMARY asserted `git grep -i 'pamplejuce' -- ':!.planning/' ':!CLAUDE.md' ':!LICENSES.md'` returns empty. My initial vendoring of cmake/ helper comments reintroduced the string in eight places (provenance comments naming `sudara/pamplejuce`). Technically a regression of plan-01's invariant, even though the comments are factually correct.
- **Fix:** Reworded each comment to attribute `sudara/cmake-includes` directly (which is the actual source repo, not the sibling Pamplejuce template). License attribution still lives in LICENSES.md, which is excluded from the grep gate exactly to absorb this kind of legitimate reference.
- **Files modified:** CMakeLists.txt, .gitmodules, cmake/Benchmarks.cmake, cmake/DukkoIPP.cmake, cmake/DukkoMacOS.cmake, cmake/DukkoVersion.cmake, cmake/JUCEDefaults.cmake, cmake/Tests.cmake
- **Commit:** 71a4f7b

### Authentication / Environment Gates

**1. cmake not installed locally**
- **Issue:** `which cmake` returned nothing on the developer's macOS machine; no Homebrew install present, no `/Applications/CMake.app`. The plan's verify gate (`cmake -B build -DCMAKE_BUILD_TYPE=Release`) cannot run.
- **Resolution:** Installed via `pip3 install --user cmake` → cmake 4.3.2 in `~/Library/Python/3.9/bin`. Used `export PATH="/Users/aernoutottens/Library/Python/3.9/bin:$PATH"` for the verify run.
- **Status:** Not a code change; informational. Plan 04 will use the macos-14 GitHub runner's pre-installed cmake.

## Known Stubs

None. The CMakeLists.txt as committed configures end-to-end; `Dukko_CLAP` and `Dukko` (VST3) targets are both created. Plan 03 wires CI; plan 04 does first-push verification.

## Threat Flags

None — no new attack surface introduced. The runtime CPM fetches (T-02-01) are mitigated by the SHA/tag pins recorded in CMakeLists.txt + LICENSES.md, exactly per the plan's threat register. The vendored cmake/ helpers are static text files, no network calls (CPM.cmake itself downloads at configure time, but the SHA256 is hardcoded — `EXPECTED_HASH SHA256=2020b4fc...` — so a tampered upstream would fail the hash check).

## Self-Check: PASSED

- `[x] CMakeLists.txt has GIT_TAG 8.0.12` — `grep -q 'GIT_TAG 8.0.12' CMakeLists.txt` exit 0
- `[x] CMakeLists.txt has GIT_TAG v2.4.0` — exit 0
- `[x] CMakeLists.txt has clap-juce-extensions` — exit 0
- `[x] clap_juce_extensions_plugin call (non-comment)` — `grep -nE '^[^#]*clap_juce_extensions_plugin\(' CMakeLists.txt` returns line 174
- `[x] CLAP_ID literal "com.dukkoaudio.dukko"` — exit 0
- `[x] PERMANENT comment block intact` — `grep -q 'PERMANENT — DO NOT CHANGE — see D-01..D-04' CMakeLists.txt` exit 0
- `[x] LICENSES.md exists` — `test -f LICENSES.md` exit 0
- `[x] LICENSES.md has both required tables` — Bundled / Linked + CI-Only Tools sections present
- `[x] LICENSES.md has GPLv3 reasoning paragraph` — `grep -qi 'does not taint' LICENSES.md` exit 0
- `[x] LICENSES.md has clap-juce-extensions SHA40` — `grep -E 'clap-juce-extensions.*[a-f0-9]{40}' LICENSES.md` matches
- `[x] LICENSES.md has all 7 deps + tools` — JUCE, clap-juce-extensions, chowdsp_utils, CPM.cmake, pluginval, clap-validator, Steinberg all `grep -qi` exit 0
- `[x] LICENSES.md has all 4 version pins` — 8.0.12, v2.4.0, 1.0.4, 0.3.2 all `grep -qi` exit 0
- `[x] QUAL-05 enumeration check passes` — every dir under build/_deps/ has a matching row in LICENSES.md (catch2, chowdsp_utils, clap-juce-extensions, juce, melatonin_inspector all OK)
- `[x] cmake -B build configures cleanly` — exits 0; produces Dukko + Dukko_CLAP targets
- `[x] Plan 01-01 grep gate not regressed` — `git grep -i 'pamplejuce' -- ':!.planning/' ':!CLAUDE.md' ':!LICENSES.md'` exits 1 (no matches)
- `[x] All three commits exist on the worktree branch` — c688c52, 1e18d76, 71a4f7b verified via `git log --oneline -5`
- `[x] No accidental file deletions in any of the three commits` — `git diff --diff-filter=D --name-only HEAD~3 HEAD` returns empty

Commit hashes verified against `git log --oneline -5`.
