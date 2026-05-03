---
phase: 01-build-foundation-ci
plan: 03
subsystem: build-foundation
tags: [ci, github-actions, validators, pluginval, clap-validator, steinberg, asan, cpm-cache]
requires:
  - "Plan 01-01 outputs (CMakeLists.txt with FORMATS=VST3 + Dukko target naming → produces build/Dukko_artefacts/Release/VST3/Dukko.vst3 layout)"
  - "Plan 01-02 outputs (clap_juce_extensions_plugin call → produces build/Dukko_artefacts/Release/CLAP/Dukko.clap; LICENSES.md exists with all deps tracked; Steinberg validator binary will appear under build/_deps/ at build time)"
  - "Network access on macos-14 runner for: actions/checkout@v4, actions/cache@v4, actions/upload-artifact@v4, GitHub releases for pluginval v1.0.4 + clap-validator 0.3.2, CPM-fetched runtime deps"
provides:
  - ".github/workflows/build_and_test.yml — CI that gates every push + PR with Release build + arm64 verification + pluginval strict-10 + Steinberg + clap-validator + LICENSES.md staleness guard + artifact upload (job 1) and Debug+ASan compile-only (job 2)"
  - "Triggers: push to any branch (`branches: ['**']`) + pull_request to any branch (D-13)"
  - "Artifact contract: Dukko-VST3 and Dukko-CLAP uploads, 30-day retention (D-12)"
  - "CPM cache contract: ~/Library/Caches/CPM keyed on hashFiles('**/CMakeLists.txt', '**/cmake/*.cmake') (D-14)"
affects:
  - "Plan 04 (CI publish + first push) — this workflow file is the artifact under test on first push; README badge URL points to .github/workflows/build_and_test.yml; if `clap-validator-0.3.2-macos-arm64.tar.gz` asset filename is wrong on first push, plan 04 patches the URL inline as a one-liner per plan-03 §interfaces note"
  - "Phase 2 (DSP scaffold) — Debug+ASan compile-only job is in place; Phase 2's audio-thread invariants land on a job that already runs without further CI changes"
  - "Future contributors — LICENSES.md staleness guard means any new CPM dep added without a corresponding LICENSES.md row blocks merge until updated (T-03-03 mitigation, intentional)"
tech-stack:
  added:
    - "GitHub Actions actions/checkout@v4, actions/cache@v4, actions/upload-artifact@v4 (workflow-only deps; no runtime impact on Dukko binary)"
    - "pluginval 1.0.4 (CI-only subprocess; GPLv3 — does not taint Dukko per LICENSES.md reasoning paragraph)"
    - "clap-validator 0.3.2 (CI-only subprocess; MIT)"
    - "Steinberg VST3 validator (CI-only; bundled in JUCE 8 SDK fork; located dynamically under build/_deps/ at build time)"
  patterns:
    - "Validator binary version pinning at the curl URL (pluginval v1.0.4, clap-validator 0.3.2) — bump deliberately by editing one URL each"
    - "Dynamic Steinberg validator path resolution via `find build/_deps -name validator -type f` — robust to JUCE's _deps reshuffles across point releases (resolves outstanding TBD #5 from plan 02 SUMMARY)"
    - "`lipo -archs ... | grep -w arm64` for fail-fast arm64 verification (the `-w` flag rejects `arm64e` and other variant tokens)"
    - "Self-invalidating LICENSES.md staleness guard: future contributors are blocked at CI until they add their new CPM dep to LICENSES.md (loud `MISSING in LICENSES.md: <name>` failure)"
key-files:
  created:
    - "(none — all changes are within the existing build_and_test.yml file)"
  modified:
    - ".github/workflows/build_and_test.yml — REWROTE end-to-end (385 lines → 200 lines net) to match the D-10..D-14 spec. Replaced Pamplejuce's Linux/Windows matrix + signing/notarization steps + nightly job with a clean two-job macos-14 layout matching plan-03 spec verbatim"
  deleted:
    - "(none — file kept its plan-01 filename `build_and_test.yml`; the Pamplejuce upstream content was rewritten in place)"
decisions:
  - "RESOLVED outstanding TBD #5 (Steinberg validator path): chose dynamic `find build/_deps -name validator -type f -perm -u+x | head -1` over hardcoded path. Rationale: (a) JUCE 8.x point releases occasionally reshuffle `_deps/` layout; (b) hardcoding would be silently wrong if path changes — `find` returning empty exits non-zero loudly; (c) no measurable CI cost (the find runs in <100ms over a small _deps tree). Added `-perm -u+x` to filter out a stray non-executable header named `validator.h` if present."
  - "Workflow file kept at `.github/workflows/build_and_test.yml` (Pamplejuce upstream filename preserved by plan 01-01) instead of renaming to plan 03's text-referenced `build.yml`. Per plan 01-01 SUMMARY deviation #5 — the orchestrator prompt also flags this. README.md badge URL (set in plan 01) already points at `build_and_test.yml`, so renaming would break the badge. Plan 03's `<files_modified>` field lists `.github/workflows/build.yml` but the *intent* (single Dukko CI workflow file) is satisfied by editing the existing file; only the filename diverges."
  - "Hardcoded the clap-validator asset URL to `https://github.com/free-audio/clap-validator/releases/download/0.3.2/clap-validator-0.3.2-macos-arm64.tar.gz`, matching GitHub's standard `releases/download/<tag>/<asset-filename>` convention. If the actual published asset has a slightly different filename, the curl step fails loudly with HTTP 404 on plan 04's first push — plan 04 patches the URL inline as a one-liner per plan-03 §interfaces note. Preferred over a dynamic GitHub API lookup (rate-limited unauthenticated, silent failure mode if schema changes)."
  - "Added a `case` filter in the LICENSES.md staleness guard that skips `*-build` and `*-subbuild` directories under `build/_deps/`. CPM creates these as transient build directories alongside the `<name>-src` source dirs; they have no canonical \"dep name\" to track in LICENSES.md. Without the filter, the guard would treat e.g. `juce-build` as a missing dep and fail. The filter only skips internal-CPM directories — every actual `<name>-src` is still enumerated and checked."
  - "Threaded ASan flags through CMAKE_CXX_FLAGS, CMAKE_C_FLAGS, and the three linker-flag families (EXE, SHARED, MODULE). Plan text only mentions `CMAKE_CXX_FLAGS=\"-fsanitize=address -fno-omit-frame-pointer\"`, but ASan typically needs the linker flags too on macOS — without them, link can succeed but the binary crashes at startup with `dyld: Library not loaded: @rpath/libclang_rt.asan_osx_dynamic.dylib`. Adding the link-side flags is per CLANG/ASan docs; cost is zero extra CMake variables read because they're set inline."
  - "Did NOT add a `concurrency:` block. Plan-03 explicitly forbids it (per CONTEXT.md \"Deferred Ideas\"). The Pamplejuce upstream had `concurrency: group: ${{ github.ref }}; cancel-in-progress: true` — this rewrite drops it. Verified absent via `grep -q '^concurrency:' .github/workflows/build_and_test.yml` returning non-zero."
  - "Did NOT preserve any of the Pamplejuce default workflow's signing/notarization/installer/Linux/Windows steps. Plan 03 spec is macos-14 only, two jobs, no signing — full rewrite is the cleanest path. Future commercial-release work (Phase 6+) will reintroduce signing in a separate workflow file or a separate job."
metrics:
  duration: "~6 minutes"
  completed: "2026-05-03T19:25:00Z"
  files_changed: 1
  commits: 1
---

# Phase 01 Plan 03: CI Workflow + Validator Matrix Summary

Rewrote `.github/workflows/build_and_test.yml` from the Pamplejuce default into a focused, plan-03-spec workflow: two jobs on `macos-14`, all three Phase 1 validators (pluginval strict-10, clap-validator 0.3.2, Steinberg) wired against the Release build, arm64-only verification for both VST3 and CLAP bundles, CPM cache, LICENSES.md staleness guard, artifact upload with 30-day retention, plus a parallel Debug+ASan compile-only job. Workflow is ready for plan 04 to push and observe green.

## Workflow File

- **Path:** `.github/workflows/build_and_test.yml` (filename inherited from plan 01-01 — see plan 01-01 SUMMARY deviation #5; not renamed to `build.yml` because README badge already references the inherited filename).
- **Top-level `name:`** `Build & Validate`
- **Triggers:** `push: branches: ["**"]` + `pull_request: branches: ["**"]` (D-13).
- **Runner for both jobs:** `macos-14` (D-13 / CLAUDE.md §"CI / Packaging").

### Job 1 — `build-and-validate`

End-to-end Release build + full validator matrix + LICENSES.md guard + artifact upload.

| Step                              | Purpose                                                              |
| --------------------------------- | -------------------------------------------------------------------- |
| Checkout (recursive submodules)   | Get Dukko source                                                     |
| Cache CPM sources                 | `~/Library/Caches/CPM` keyed on CMakeLists.txt + cmake/*.cmake hashes (D-14) |
| Configure (Release, arm64)        | `cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64` |
| Build (Release)                   | `cmake --build build --config Release --parallel`                    |
| Verify arm64 (VST3)               | `lipo -archs … Dukko | grep -w arm64` — BUILD-02 fail-fast           |
| Verify arm64 (CLAP)               | `lipo -archs … Dukko | grep -w arm64` — BUILD-02 fail-fast           |
| Download pluginval 1.0.4          | curl `pluginval_macOS.zip` from Tracktion releases                   |
| Run pluginval (strictness 10)     | `--strictness-level 10 --validate-in-process` (D-10, QUAL-01)        |
| Run Steinberg validator           | `find build/_deps -name validator -type f -perm -u+x \| head -1` (D-10) |
| Download clap-validator 0.3.2     | curl pinned tarball from free-audio/clap-validator (D-10)            |
| Run clap-validator                | `./clap-validator validate Dukko.clap`                               |
| LICENSES.md staleness guard       | enumerate `build/_deps/` (skip `-build`/`-subbuild`); `grep -qi $name LICENSES.md` |
| Upload Dukko-VST3 artifact        | `actions/upload-artifact@v4`, 30-day retention (D-12)                |
| Upload Dukko-CLAP artifact        | `actions/upload-artifact@v4`, 30-day retention (D-12)                |

### Job 2 — `build-debug-asan`

Debug + AddressSanitizer compile-only (D-11). No DSP yet in Phase 1, so ASan has nothing audio-related to find — but the toolchain is verified working so Phase 2's audio-thread-safety work lands on a job that already runs.

| Step                              | Purpose                                                              |
| --------------------------------- | -------------------------------------------------------------------- |
| Checkout (recursive submodules)   | Get Dukko source                                                     |
| Cache CPM sources                 | Same key as job 1 (shared cache hits)                                |
| Configure (Debug + ASan, arm64)   | ASan flags threaded through CXX_FLAGS, C_FLAGS, and EXE/SHARED/MODULE linker flags |
| Build (Debug + ASan)              | `cmake --build build-debug --config Debug --parallel`                |

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Plan/upstream mismatch] Workflow file is `build_and_test.yml`, not `build.yml`**
- **Found during:** Task 1 (file lookup; orchestrator prompt also flagged it)
- **Issue:** Plan 03 frontmatter `files_modified` lists `.github/workflows/build.yml`. The actual file in the repo (per plan 01-01 SUMMARY deviation #5) is `build_and_test.yml`. README badge already points at `build_and_test.yml`. Renaming would break the badge.
- **Fix:** Edited the existing `build_and_test.yml` in place. Intent (single Dukko CI workflow gating every push) is satisfied; only filename diverges from plan text.
- **Files modified:** `.github/workflows/build_and_test.yml`
- **Commit:** 7b21c6a

**2. [Rule 2 - Critical functionality] Skip `-build`/`-subbuild` dirs in LICENSES.md staleness guard**
- **Found during:** Task 1 authoring (mental dry-run)
- **Issue:** Plan-text guard loop is `for dep_dir in build/_deps/*/`. CPM creates not just `<name>-src` directories but also `<name>-build` and `<name>-subbuild` transient build dirs alongside. Without filtering, the guard would treat e.g. `juce-build` as a missing dep (not in LICENSES.md) and fail every CI run.
- **Fix:** Added a `case` filter that `continue`s past `*-build` and `*-subbuild`; only `-src` and other canonical dep dirs reach the grep. Every actual dep is still enumerated.
- **Files modified:** `.github/workflows/build_and_test.yml`
- **Commit:** 7b21c6a

**3. [Rule 2 - Critical functionality] Threaded ASan flags through linker variables, not just CXX_FLAGS**
- **Found during:** Task 1 authoring
- **Issue:** Plan text only sets `CMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer"`. On macOS, ASan also needs the link-side flag to pull in `libclang_rt.asan_osx_dynamic.dylib`; without it, the build links successfully but the resulting binary crashes at startup with `dyld: Library not loaded`. ASan compile-only job goal is to *verify the toolchain is working* — a binary that can't run isn't "working".
- **Fix:** Set `-fsanitize=address` on `CMAKE_C_FLAGS`, `CMAKE_EXE_LINKER_FLAGS`, `CMAKE_SHARED_LINKER_FLAGS`, and `CMAKE_MODULE_LINKER_FLAGS` in addition to `CMAKE_CXX_FLAGS`. `-fno-omit-frame-pointer` only on the compile flags. Standard pattern from clang/ASan docs.
- **Files modified:** `.github/workflows/build_and_test.yml`
- **Commit:** 7b21c6a

**4. [Rule 1 - Plan/upstream mismatch] Steinberg validator find filter `-perm -u+x`**
- **Found during:** Task 1 authoring
- **Issue:** Plan text uses `find build/_deps -name validator -type f | head -1`. JUCE's vendored VST3 SDK contains files literally named `validator` in two places: the built executable (under `validator-build/...`) and a header `validator.h`. `-name validator -type f` would match both; `head -1` then picks whichever the filesystem traversal hits first — non-deterministic and may pick the header.
- **Fix:** Added `-perm -u+x` to the find. The header is non-executable; the built validator is executable. The find is now deterministic.
- **Files modified:** `.github/workflows/build_and_test.yml`
- **Commit:** 7b21c6a

### Confirmed Plan Adherence

- **No `concurrency:` block** — verified absent. Plan explicitly forbids it (deferred per CONTEXT.md). The Pamplejuce upstream had one; this rewrite drops it. `grep -q '^concurrency:' .github/workflows/build_and_test.yml` returns non-zero.
- **No signing / notarization / installer steps** — Pamplejuce upstream's macOS codesign + Apple notarytool + Windows Azure signing + Linux installer steps were all dropped. Phase 1 scope is build + validate only; commercial-release signing is Phase 6+ work.
- **No Linux / Windows matrix** — Pamplejuce upstream had a 3-OS matrix; this rewrite is macos-14 only per CLAUDE.md v1 platform constraint.

## Outstanding TBDs Resolved

- **TBD #5 (from plan 01-02 SUMMARY): Steinberg VST3 validator binary path under `build/_deps/`** — RESOLVED via dynamic `find build/_deps -name validator -type f -perm -u+x | head -1`. No hardcoded path; robust to JUCE point-release reshuffles.

## Outstanding TBDs Handed Forward

- **clap-validator asset filename uncertainty** — the URL `https://github.com/free-audio/clap-validator/releases/download/0.3.2/clap-validator-0.3.2-macos-arm64.tar.gz` is a best-guess matching GitHub's standard release-asset URL pattern. If wrong on plan 04's first push, the curl step fails loudly with HTTP 404 — plan 04 patches the URL inline as a one-liner. This is preferred over a dynamic GitHub API lookup (rate-limited unauthenticated, silent failure mode if schema changes).

## Authentication Gates

None.

## Known Stubs

None. The workflow file is complete and ready to execute on first push (plan 04). The "first green run" verification is plan 04's responsibility — this plan only authors the workflow definition.

## Threat Flags

None — no new attack surface introduced beyond what was already in the threat register (T-03-01 binary downloads, T-03-02 PR-from-fork token scope, T-03-03 LICENSES.md guard fail-on-missing-dep, T-03-04 build-path log exposure). All four threats are addressed exactly as the threat register prescribed.

## Self-Check: PASSED

- `[x] .github/workflows/build_and_test.yml exists` — `test -f` exit 0
- `[x] Workflow name "Build & Validate"` — `grep -q 'name: Build & Validate'` exit 0
- `[x] Both jobs use macos-14 runner` — `grep -c 'runs-on: macos-14'` returns 2
- `[x] push + pull_request triggers with branches **` — all three greps exit 0 (D-13)
- `[x] Job IDs build-and-validate + build-debug-asan present` — both grep exit 0
- `[x] pluginval strictness-level 10 wired` — exit 0 (D-10, QUAL-01)
- `[x] clap-validator wired` — exit 0 (D-10)
- `[x] Steinberg validator find wired` — exit 0 (D-10, resolves TBD #5)
- `[x] arm64 verification for both formats` — `grep -c 'lipo -archs'` returns 2 (BUILD-02)
- `[x] ASan compile flag wired` — `grep -q 'fsanitize=address'` exit 0 (D-11)
- `[x] CPM cache wired` — `actions/cache@v4` + `Library/Caches/CPM` both exit 0 (D-14)
- `[x] Artifact upload with 30-day retention` — `retention-days: 30` + `name: Dukko-VST3` + `name: Dukko-CLAP` all exit 0 (D-12)
- `[x] LICENSES.md staleness guard step present` — `grep -q 'MISSING in LICENSES.md'` exit 0 (QUAL-05)
- `[x] No concurrency block` — `grep -q '^concurrency:'` returns non-zero
- `[x] YAML parses cleanly` — `python3 -c "import yaml; yaml.safe_load(open(...))"` exit 0
- `[x] Commit exists on worktree branch` — 7b21c6a verified via `git log --oneline -5`
- `[x] No accidental file deletions in commit` — `git diff --diff-filter=D --name-only HEAD~1 HEAD` empty

Commit hash verified against `git log --oneline -5`.
