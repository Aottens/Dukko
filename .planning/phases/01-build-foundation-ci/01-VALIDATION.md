---
phase: 1
slug: build-foundation-ci
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-05-03
---

# Phase 1 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.
> Phase 1 is a build-scaffold phase — no unit-test framework is introduced. Validation is performed via CI-orchestrated plugin validators (pluginval, clap-validator, Steinberg validator) plus mechanical bundle-inspection checks.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | None (build scaffold phase). Validation = plugin validators + shell assertions |
| **Config file** | `.github/workflows/build.yml` (Wave 0 creates this) |
| **Quick run command** | `cmake --build build --config Release && lipo -archs build/Dukko_artefacts/Release/VST3/Dukko.vst3/Contents/MacOS/Dukko \| grep -w arm64` |
| **Full suite command** | `git push` → wait for `macos-14` Actions job (Build & Validate + Build Debug+ASan) to go green |
| **Estimated runtime** | Local quick: ~30s after first build (CPM cache warm). Full CI suite: ~6–8 min cold, ~2–3 min cache-warm (per D-14) |

---

## Sampling Rate

- **After every task commit:** Run the quick command (build + lipo arm64 check) when the task touches `CMakeLists.txt`, `cmake/*.cmake`, or `source/*`. Skip for doc-only commits (`LICENSES.md`, `README.md`, `.gitignore`).
- **After every plan wave:** `git push` to trigger CI; wait for green badge before starting next wave.
- **Before `/gsd-verify-work`:** Full CI suite green AND manual Bitwig load-test passes (drag VST3 + CLAP into Bitwig, both appear under "Dukko Audio › Dukko").
- **Max feedback latency:** Local quick check < 60s; CI full suite < 8 min.

---

## Per-Task Verification Map

> Tasks are not yet enumerated — populated by `gsd-planner` in PLAN.md files. Each requirement maps to one or more tasks; every task in the plan must list its automated verification command in `<acceptance_criteria>`.

| Req ID | Behavior | Test Type | Automated Command | File Exists | Status |
|--------|----------|-----------|-------------------|-------------|--------|
| BUILD-01 | CMake build produces VST3 bundle | Smoke | `cmake -B build && cmake --build build --config Release && test -d build/Dukko_artefacts/Release/VST3/Dukko.vst3` | ❌ W0 (CMakeLists.txt rename + CPM blocks) | ⬜ pending |
| BUILD-02 | VST3 binary is native arm64 | Automated | `lipo -archs build/Dukko_artefacts/Release/VST3/Dukko.vst3/Contents/MacOS/Dukko \| grep -w arm64` | ❌ W0 | ⬜ pending |
| BUILD-03 | CLAP bundle produced alongside VST3 | Smoke | `test -d build/Dukko_artefacts/Release/CLAP/Dukko.clap && lipo -archs build/Dukko_artefacts/Release/CLAP/Dukko.clap/Contents/MacOS/Dukko \| grep -w arm64` | ❌ W0 (clap-juce-extensions CPM + glue) | ⬜ pending |
| BUILD-04 | CI runs on every push, builds VST3+CLAP on macos-14 | Automated | GitHub Actions job exit code 0 (verified by green badge / `gh run list` after push) | ❌ W0 (`.github/workflows/build.yml`) | ⬜ pending |
| BUILD-05 | No "KickstartClone" or "Pamplejuce" leftovers anywhere | Automated | `git grep -i 'kickstartclone\|pamplejuce' -- ':!.planning/' ':!CLAUDE.md' ':!LICENSES.md'` returns empty AND `plutil -p build/Dukko_artefacts/Release/VST3/Dukko.vst3/Contents/Info.plist \| grep -E 'CFBundleIdentifier.*com\.dukkoaudio\.dukko'` matches | ❌ W0 | ⬜ pending |
| QUAL-01 | pluginval strict-10 passes on every CI build | Automated | `./pluginval.app/Contents/MacOS/pluginval --strictness-level 10 --validate-in-process build/Dukko_artefacts/Release/VST3/Dukko.vst3` exits 0 | ❌ W0 (pluginval download step) | ⬜ pending |
| QUAL-05 | LICENSES.md exists and lists all CPM-resolved deps | Automated | `test -f LICENSES.md && for d in build/_deps/*/; do n=$(basename "$d" \| sed 's/-src$//'); grep -qi "$n" LICENSES.md \|\| { echo "MISSING: $n"; exit 1; }; done` | ❌ W0 (`LICENSES.md` at repo root) | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

### Bonus per-D-10 validations (CI gates beyond minimum requirement set)

| Behavior | Source | Automated Command | Status |
|----------|--------|-------------------|--------|
| clap-validator passes against Release CLAP | D-10 | `./clap-validator validate build/Dukko_artefacts/Release/CLAP/Dukko.clap` exits 0 | ⬜ pending |
| Steinberg VST3 validator passes against Release VST3 | D-10 | `"$(find build/_deps -name validator -type f \| head -1)" build/Dukko_artefacts/Release/VST3/Dukko.vst3` exits 0 | ⬜ pending |
| Debug+ASan job compiles cleanly | D-11 | `cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer" && cmake --build build-debug --config Debug` exits 0 | ⬜ pending |
| Workflow uploads VST3 + CLAP artifacts | D-12 | `gh run download <run-id>` retrieves both `Dukko-VST3` and `Dukko-CLAP` artifacts | ⬜ pending |
| CPM cache warm on second push | D-14 | Compare cold vs warm CI duration; warm < 50% of cold | ⬜ pending |

---

## Wave 0 Requirements

All test infrastructure is greenfield — Wave 0 must create:

- [ ] `CMakeLists.txt` — Pamplejuce-derived, with Dukko identifiers and CPM blocks (JUCE 8.0.12, clap-juce-extensions @ SHA, chowdsp_utils v2.4.0)
- [ ] `.github/workflows/build.yml` — Release build + validators job + Debug+ASan compile-only job, with CPM cache + arm64 verification + artifact upload
- [ ] `LICENSES.md` at repo root — hand-curated table per D-17
- [ ] `VERSION` file at repo root — `0.1.0`
- [ ] `.gitignore` — verify Pamplejuce default covers build dirs, .DS_Store, *.xcodeproj/, _deps/, cmake-build-*/, .idea/; add any missing
- [ ] `README.md` — Dukko description + CI status badge
- [ ] pluginval download step in CI (no framework install — direct binary download from GitHub releases)
- [ ] clap-validator download step in CI

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Plugin loads in Bitwig as native arm64 | BUILD-02, SC1 | Bitwig is the primary acceptance host; no headless Bitwig host-load test exists. Verifying actual host-load is part of Phase 1 success criterion #1. | 1. Build locally (`COPY_PLUGIN_AFTER_BUILD=TRUE` installs to `~/Library/Audio/Plug-Ins/...`). 2. Open Bitwig (latest stable). 3. Open the plugin browser; search for "Dukko". 4. Confirm both VST3 and CLAP variants appear under vendor "Dukko Audio". 5. Drag both formats onto an empty audio track; confirm each loads without error. 6. Activity Monitor → confirm Bitwig process is `Apple` (arm64), not `Intel` (no Rosetta). |
| Plugin browser shows correct vendor / name strings | BUILD-05, SC2 | Visual check of host UI; cannot be scripted against Bitwig. | Step 3 above — vendor must read "Dukko Audio", plugin must read "Dukko". No "Pamplejuce" or "Demo" variants visible. |
| README CI badge renders green on GitHub | BUILD-04 | Visual check of GitHub repo landing page after first push. | Open `https://github.com/<your-org>/Dukko` in browser; badge in README must show "Build & Validate: passing" in green. |

---

## Validation Sign-Off

- [ ] All tasks have `<acceptance_criteria>` with verifiable commands or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without an automated verify (CI run counts as verify when wave finishes)
- [ ] Wave 0 covers all ❌ W0 references in the per-task map
- [ ] No watch-mode flags
- [ ] Feedback latency < 60s (local quick) and < 8 min (CI full)
- [ ] `nyquist_compliant: true` set in frontmatter once planner has wired all tasks to entries above

**Approval:** pending
