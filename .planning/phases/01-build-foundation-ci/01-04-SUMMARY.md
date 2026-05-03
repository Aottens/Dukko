---
phase: 01-build-foundation-ci
plan: 04
subsystem: build-foundation
tags: [end-to-end-verification, build, pluginval, github-repo, badge-substitution, phase-close]
requires:
  - "Plan 01-01 outputs (Pamplejuce-rename + D-01..D-04 baked into CMakeLists.txt; README.md with <your-org> placeholder pointing at build_and_test.yml)"
  - "Plan 01-02 outputs (CPM deps wired; cmake -B build configures; Dukko + Dukko_CLAP targets; LICENSES.md complete)"
  - "Plan 01-03 outputs (.github/workflows/build_and_test.yml with all three validators + ASan job)"
  - "User-provided prerequisites (orchestrator context): CMake 4.3.2 + Ninja 1.13.2 in /opt/homebrew/bin; GitHub repo Aottens/Dukko exists (public); git remote origin already set on main; gh CLI authenticated as Aottens"
provides:
  - "Verified end-to-end Release build: cmake -B build && cmake --build build produces both Dukko.vst3 and Dukko.clap on Apple Silicon (arm64)"
  - "Verified COPY_PLUGIN_AFTER_BUILD installs both bundles to ~/Library/Audio/Plug-Ins/{VST3,CLAP}/ (TBD #4 resolved — covers BOTH formats)"
  - "Verified JUCE auto ad-hoc-signs both bundles with Identifier=com.dukkoaudio.dukko (TBD #5 resolved — no extra codesign command needed)"
  - "Verified pluginval strict-10 PASSES on both the unmodified Pamplejuce shell AND the post-fix processor (Pitfall 3 DID trigger via clap-validator's chunked state tests, fixed by writing a named ValueTree in getStateInformation/setStateInformation — see commit 5ff5141)"
  - "Verified actual VST3/CLAP build paths match the conventional Pamplejuce paths the workflow YAML hardcodes (TBD #6 resolved — workflow needs no edit)"
  - "README.md badge URL points at https://github.com/Aottens/Dukko/actions/workflows/build_and_test.yml (substitution committed)"
affects:
  - "Orchestrator post-merge: must run `git push origin main` from the main checkout, then `gh run watch` (or `gh run list` + `gh run view`) to confirm CI green; only then are BUILD-04 + QUAL-01 first-push closures complete"
  - "User: must perform manual Bitwig load test (drag VST3 + CLAP from ~/Library/Audio/Plug-Ins onto an empty audio track in Bitwig; verify both appear under vendor 'Dukko Audio' and instantiate; Activity Monitor shows Bitwig as Apple arm64). This is the ONLY remaining BUILD-02 verification gate not closed inside this plan"
  - "Phase 2 (DSP scaffold) — chowdsp_plugin_state can replace JUCE's stub state handling; pluginval strict-10 already green so Phase 2 must keep it green when adding parameters/state"
tech-stack:
  added: []
  patterns: []
key-files:
  created:
    - ".planning/phases/01-build-foundation-ci/01-04-SUMMARY.md (this file)"
    - ".planning/phases/01-build-foundation-ci/PHASE-SUMMARY.md (Phase 1 retrospective)"
  modified:
    - "README.md — substituted Aottens for <your-org> in badge URL (both occurrences in one line)"
    - ".github/workflows/build_and_test.yml — post-merge fixes: cap parallelism, scope to Dukko_VST3+Dukko_CLAP, drop Steinberg validator step, correct clap-validator URL/path (commits 6e784c1, d407aa4, f2a92e5, 9ee46fe)"
    - "source/PluginProcessor.cpp — post-merge fix: write round-trippable ValueTree state (Pitfall 3, commit 5ff5141)"
  deleted: []
decisions:
  - "Pitfall 3 DID trigger via clap-validator (not pluginval) — JUCE's VST3 wrapper accepts empty state but clap-juce-extensions' CLAP wrapper does chunked-read state tests that fail on empty MemoryBlocks. Resolved post-merge in commit 5ff5141 by writing a named ValueTree in getStateInformation/setStateInformation. Phase 2 should replace this with chowdsp_plugin_state for versioned recall."
  - "TBD #6 (CLAP/VST3 output paths) RESOLVED to the conventional Pamplejuce paths the workflow already references — no .github/workflows/build_and_test.yml edit needed: build/Dukko_artefacts/Release/VST3/Dukko.vst3 and build/Dukko_artefacts/Release/CLAP/Dukko.clap match exactly."
  - "TBD #4 (COPY_PLUGIN_AFTER_BUILD covers CLAP) RESOLVED yes — JUCE's install-driver script copied BOTH bundles to ~/Library/Audio/Plug-Ins/{VST3,CLAP}/ in a single build invocation. No add_custom_command(POST_BUILD) for CLAP needed."
  - "TBD #5 (ad-hoc codesign) RESOLVED — JUCE's COPY_PLUGIN_AFTER_BUILD path automatically runs `codesign --sign -` on both bundles (visible in build log: 'Replacing invalid signature with ad-hoc signature' twice). `codesign -dv` confirms Signature=adhoc on both installed bundles. No post-build codesign add_custom_command needed."
  - "Tasks 2 (GitHub repo creation) was SATISFIED externally by user before agent spawn (orchestrator context: Aottens/Dukko exists public; git remote origin set; gh authenticated). Task 2 checkpoint:human-action was skipped per orchestrator instructions."
  - "Tasks 3b (git push origin main) and 3c (gh run watch + Bitwig manual load) DEFERRED out of this worktree per orchestrator instructions. Push must happen from the main checkout AFTER worktree merge; Bitwig load is a human-only verification that cannot run inside an agent. Both are documented as PENDING below for the orchestrator + user to close out."
metrics:
  duration: "~6 minutes (cmake configure ~62s + Release build ~2.5min cold + pluginval strict-10 ~1.5min + verifications + commit)"
  completed: "2026-05-03T17:22:32Z"
  files_changed: 1
  commits: 1
---

# Phase 01 Plan 04: End-to-End Verification + Phase 1 Close Summary

Ran the first end-to-end Release build of Dukko on Apple Silicon: both VST3 and CLAP bundles produced as native arm64, both auto-installed to `~/Library/Audio/Plug-Ins/`, both ad-hoc-signed by JUCE's COPY_PLUGIN_AFTER_BUILD path, and pluginval strict-10 PASSED on the VST3 against the unmodified Pamplejuce-shell processor. README badge URL substituted from `<your-org>` to `Aottens`. Three execution-time TBDs (#4, #5, #6) resolved cleanly; Pitfall 3 (state-round-trip) did NOT trigger.

Two manual/external steps deferred to the orchestrator/user (per orchestrator context): the first push from `main` + CI watch, and the Bitwig manual host-load test. Until those run, BUILD-02 (Bitwig manual) and the first-push portion of BUILD-04 + QUAL-01 are PENDING — but everything inside the agent's reachable scope is verified GREEN.

## Recorded Outputs

### GitHub repo URL

- **Repo:** https://github.com/Aottens/Dukko (public, owner = Aottens)
- **Workflow filename:** `.github/workflows/build_and_test.yml` (NOT `build.yml` — see plan 01-01 SUMMARY deviation #5)
- **Badge URL** (now in README.md): `https://github.com/Aottens/Dukko/actions/workflows/build_and_test.yml/badge.svg`

### Actual VST3 / CLAP build paths (TBD #6 resolved)

| Bundle      | Path                                                 |
| ----------- | ---------------------------------------------------- |
| VST3        | `build/Dukko_artefacts/Release/VST3/Dukko.vst3`      |
| CLAP        | `build/Dukko_artefacts/Release/CLAP/Dukko.clap`      |

These match the conventional Pamplejuce paths the plan-03 workflow already references — no `.github/workflows/build_and_test.yml` edit needed.

### COPY_PLUGIN_AFTER_BUILD behaviour for CLAP (TBD #4 resolved)

JUCE's install path copied BOTH bundles in one build invocation. Build-log evidence:

```
Installing /...build/Dukko_artefacts/Release/CLAP/Dukko.clap to /Users/aernoutottens/Library/Audio/Plug-Ins/CLAP/
[100%] Built target Dukko_CLAP
...
-- Installing: /Users/aernoutottens/Library/Audio/Plug-Ins/VST3/Dukko.vst3
-- Installing: /Users/aernoutottens/Library/Audio/Plug-Ins/VST3/Dukko.vst3/Contents
-- Installing: /Users/aernoutottens/Library/Audio/Plug-Ins/VST3/Dukko.vst3/Contents/_CodeSignature
... [full bundle tree copied]
[100%] Built target Dukko_VST3
```

Filesystem confirmation:

```
$ ls -la ~/Library/Audio/Plug-Ins/VST3/Dukko.vst3
drwxr-xr-x  3 aernoutottens  staff  96 May  3 19:21 .
$ ls -la ~/Library/Audio/Plug-Ins/CLAP/Dukko.clap
drwxr-xr-x  3 aernoutottens  staff  96 May  3 19:21 .
```

No `add_custom_command(POST_BUILD ...)` for CLAP installation needed. `COPY_PLUGIN_AFTER_BUILD TRUE` (D-09) handles both formats out of the box.

### Codesign behaviour (TBD #5 resolved)

JUCE auto-applies an ad-hoc signature during the COPY_PLUGIN_AFTER_BUILD path. Build-log evidence:

```
/...Dukko.vst3: code has no resources but signature indicates they must be present
-- Replacing invalid signature with ad-hoc signature
/...Dukko.vst3: replacing existing signature
removing moduleinfo.json
creating /...Dukko.vst3
/...Dukko.vst3: a sealed resource is missing or invalid
-- Replacing invalid signature with ad-hoc signature
/...Dukko.vst3: replacing existing signature
```

`codesign -dv` confirmation:

```
$ codesign -dv ~/Library/Audio/Plug-Ins/VST3/Dukko.vst3
Identifier=com.dukkoaudio.dukko
Signature=adhoc
TeamIdentifier=not set

$ codesign -dv ~/Library/Audio/Plug-Ins/CLAP/Dukko.clap
Identifier=Dukko
Signature=adhoc
TeamIdentifier=not set
```

Both bundles report `Signature=adhoc` — the Apple Silicon Gatekeeper minimum (per RESEARCH.md Pitfall 4) is satisfied automatically. No `add_custom_command(POST_BUILD ... codesign --sign - --force ...)` needed. The CLAP bundle's `Identifier=Dukko` is the JUCE-generated CFBundleIdentifier-style identifier from clap-juce-extensions — distinct from the VST3's `com.dukkoaudio.dukko`, which is the explicit BUNDLE_ID set in CMakeLists.txt. The CLAP plugin's stable cross-host ID is `com.dukkoaudio.dukko` (set via `clap_juce_extensions_plugin(... CLAP_ID "com.dukkoaudio.dukko" ...)`); the bundle Identifier above is the macOS-bundle-level identifier, which is different (and harmless — Bitwig loads the CLAP by its CLAP_ID, not by its bundle Identifier).

### Bundle identifier sanity check (D-04 → binary)

```
$ plutil -p build/Dukko_artefacts/Release/VST3/Dukko.vst3/Contents/Info.plist | grep CFBundleIdentifier
  "CFBundleIdentifier" => "com.dukkoaudio.dukko"
```

D-04 propagated from CMakeLists.txt → JUCE → VST3 Info.plist correctly. No drift.

### pluginval strict-10 outcome (Pitfall 3 NOT triggered)

**Result: PASSED out of the box on the unmodified Pamplejuce shell.**

Command run:

```bash
/tmp/pluginval.app/Contents/MacOS/pluginval \
  --strictness-level 10 --validate-in-process \
  "build/Dukko_artefacts/Release/VST3/Dukko.vst3"
```

Final output: `SUCCESS` with exit 0. All test categories ran: General Tests, Editor, Plugin State, State, Background thread state, Parameter thread safety, auval, vst3 validator (skipped — no path set, expected; CI step explicitly invokes Steinberg validator), Basic bus, Listing available buses, Enabling all buses, Disabling non-main busses, Restoring default layout, Fuzz parameters. State round-trip tests at all sample rates (44.1/48/96 kHz × multiple block sizes including 64/128/256/512/1024) passed cleanly.

**Significance:** Pitfall 3 documented in 01-RESEARCH.md (state round-trip failure on the unmodified Pamplejuce shell) did NOT manifest. The JUCE template's default `getStateInformation` / `setStateInformation` implementations round-trip an empty `MemoryBlock` correctly under pluginval's strict-10 fuzzing. **NO source/PluginProcessor.cpp modification was made.** Phase 2 still has a clean opportunity to replace the JUCE stubs with `chowdsp_plugin_state` for versioned state recall — but that is now a planned enhancement for parameter-system work, not a Phase-1 emergency fix.

QUAL-01's "pluginval strict-10 must pass" gate is therefore CLOSED locally; the CI portion is closed when the orchestrator pushes and the workflow runs green.

### First Actions run URL + conclusion

**RESOLVED — green after 5 fix commits.**

- **First green run:** https://github.com/Aottens/Dukko/actions/runs/25288354439 (head SHA `5ff5141`)
- **Conclusion:** `success` (both jobs)
- **Duration:** ~3.5 min Release + validators, ~2 min Debug+ASan (warm CPM cache)

Required orchestrator-side fixes between worktree-merge push and first green:

| # | Commit | Cause |
|---|--------|-------|
| 1 | `6e784c1` fix(01-04): cap CI parallelism + scope build to Dukko_All to prevent OOM | Run 25285852022 deadlocked at `Built target Dukko_vst3_helper` for 60 min. Pamplejuce ships `Tests` + `Benchmarks` as top-level targets each linking their own JUCE compile; unbounded `--parallel` on the macos-14 runner (3 cores / 14 GB) tried 3 simultaneous JUCE trees → OOM swap thrash. Fix: `--target Dukko_All --parallel 3`. |
| 2 | `d407aa4` fix(01-04): explicitly build Dukko_CLAP target in CI | `Dukko_All` only contains formats from `juce_add_plugin`'s `FORMATS` list (VST3 here). The CLAP target is added later by `clap_juce_extensions_plugin` and is NOT in `Dukko_All`. Fix: `--target Dukko_VST3 --target Dukko_CLAP`. |
| 3 | `f2a92e5` fix(01-04): defer Steinberg VST3 validator to Phase 6 | Plan 01-03's executor wired the Steinberg step on the assumption JUCE's vendored VST3 SDK exposes a `validator` CMake target. **It does not** — JUCE bundles SDK headers but not the standalone tool. RESEARCH.md TBD #5 had asked exactly this and was incorrectly resolved. Per ROADMAP, Steinberg validator is Phase 6's gate (QUAL-02); restored that split. Phase 1 keeps pluginval + clap-validator. |
| 4 | `9ee46fe` fix(01-04): correct clap-validator asset name + binary path | Plan 01-03's executor guessed `clap-validator-0.3.2-macos-arm64.tar.gz`. Actual release asset is `clap-validator-0.3.2-macos-universal.tar.gz` (universal binary), and the tarball expands to `binaries/clap-validator`, not the bare `clap-validator`. Fix: corrected URL + `binaries/` prefix on chmod and run paths. |
| 5 | `5ff5141` fix(01-04): write round-trippable state in PluginProcessor (Pitfall 3) | clap-validator's `state-reproducibility-{basic,flush,null-cookies}` all FAILED — `clap_plugin_state::load()` returned false because the Pamplejuce stub `getStateInformation` produced an empty MemoryBlock. Plan 01-04 Task 1 Step 6 documented this fix verbatim, referencing RESEARCH.md Pitfall 3. **The original 01-04-SUMMARY's claim that "Pitfall 3 did NOT trigger" was wrong** — it was only true against pluginval's VST3 path (the JUCE VST3 wrapper is permissive about empty state); the CLAP wrapper does stricter chunked-read tests that exposed the real issue. Fixed by writing a named ValueTree on save and reading it back on load. Phase 2's chowdsp_plugin_state will replace these stubs with versioned state. |

Lessons for future phases:
- **Plan 03 author "verified" the workflow with `python3 -c yaml.safe_load(...)` but never tested an actual GitHub Actions run.** YAML validity ≠ workflow correctness. Future CI-authoring plans should require a dry-run push and observe before declaring done.
- **TBD #5 (Steinberg validator path) was resolved by guessing a `find` glob without verification.** Future TBDs whose resolution requires runtime evidence must explicitly demand it (configure + inspect, not just plan-stage research).
- **Pitfall 3 IS triggered by clap-validator even when pluginval passes.** Phase 2 should treat the JUCE-template state stubs as load-bearing — the current minimal ValueTree write is sufficient for Phase 1 but chowdsp_plugin_state is the proper fix.

### Bitwig manual load result (VST3 + CLAP)

**PENDING — deferred to user (human-only verification per VALIDATION.md "Manual-Only Verifications").**

Per orchestrator context:

> "Bitwig manual load (Task 3 step 5): Cannot be performed inside this agent — it is a human-only verification. Document it as PENDING in SUMMARY.md, do NOT block on it."

User to perform:

1. Open Bitwig (latest stable).
2. Plugin browser → search "Dukko".
3. Confirm BOTH variants appear under vendor "Dukko Audio": one VST3, one CLAP.
4. Drag the VST3 onto an empty audio track. Confirm it instantiates without error.
5. Drag the CLAP onto a separate empty audio track. Confirm it instantiates without error.
6. Activity Monitor → find Bitwig process → confirm "Kind" column reads "Apple" (arm64), not "Intel" (Rosetta).
7. README badge: open https://github.com/Aottens/Dukko in a browser. The "Build & Validate: passing" badge must render green at the top of the README (this depends on step "First Actions run" above going green first).

The bundles are already installed at `~/Library/Audio/Plug-Ins/VST3/Dukko.vst3` and `~/Library/Audio/Plug-Ins/CLAP/Dukko.clap` (the build agent confirmed both via `ls -la`), so no additional install step is needed before opening Bitwig.

If steps 1–7 all pass, BUILD-02 is fully closed and Phase 1 is COMPLETE.

### STATE.md / ROADMAP.md update confirmation

**Per orchestrator instructions: NOT modified by this agent.** STATE.md, ROADMAP.md, and REQUIREMENTS.md are owned by the orchestrator. After merging this worktree, the orchestrator runs the `gsd-sdk` state/roadmap/requirements handlers to:

- Increment Current Plan past 04, set Phase 1 status to completed
- Add to Decisions: JUCE 8.0.12 / clap-juce-extensions @ e8de9e8 / chowdsp_utils v2.4.0 (the actual three runtime dep pins recorded in plan 02 SUMMARY)
- Mark BUILD-02 complete (after user reports Bitwig load PASS)
- Mark BUILD-04 + QUAL-01 first-push portion complete (after orchestrator confirms CI green)

## Deviations from Plan

### Auto-fixed Issues

None. The plan executed as specified given the orchestrator-provided context (Task 2 was satisfied externally; Task 3b/c were deferred). No automatic fixes were needed during build, install, or verification.

### Plan-step Modifications (per orchestrator context)

**1. Task 2 (checkpoint:human-action) SKIPPED**
- **Per orchestrator context:** Aottens/Dukko exists, git remote `origin` is set, gh CLI is authenticated. Task 2's purpose was to obtain the GitHub owner slug + ensure the repo exists; both are pre-satisfied.
- **Action:** Proceeded directly from Task 1 to Task 3 with `Aottens` as the substituted slug.

**2. Task 3 step 3 (`git push -u origin main`) DEFERRED**
- **Per orchestrator context:** Push must happen from the main checkout AFTER worktree merge, not from this worktree's `worktree-agent-*` branch. Pushing from the worktree branch would push the wrong ref to GitHub.
- **Action:** Did NOT run `git push`. Documented as PENDING for the orchestrator above.

**3. Task 3 step 4 (`gh run watch`) DEFERRED**
- **Per orchestrator context:** The CI run can only start after the post-merge push. There is no CI run to watch from this worktree.
- **Action:** Did NOT run `gh run watch`. Documented as PENDING for the orchestrator above.

**4. Task 3 step 5 (Bitwig manual load) DEFERRED**
- **Per orchestrator context:** Human-only verification.
- **Action:** Did NOT attempt automation. Documented as PENDING for the user above with explicit instructions.

**5. Task 3 step 6 (STATE.md update) DEFERRED**
- **Per orchestrator instructions in prompt:** "Do NOT update STATE.md or ROADMAP.md — the orchestrator owns those writes."
- **Action:** Did NOT touch STATE.md, ROADMAP.md, or REQUIREMENTS.md. Documented expected updates above for the orchestrator's reference.

## Authentication Gates

None encountered. The orchestrator pre-handled all auth (gh CLI authenticated, git remote configured). Tools used (`cmake`, `lipo`, `codesign`, `plutil`, `pluginval`) needed no auth.

## Known Stubs

None functional in Dukko's own source — the JUCE-template stub `getStateInformation` / `setStateInformation` in `source/PluginProcessor.cpp` are intentional placeholders that pluginval strict-10 already round-trips correctly. Phase 2 will replace them with `chowdsp_plugin_state` for versioned state, but that is enhancement work, not stub-removal.

## Threat Flags

None. No new attack surface introduced. The acted-on threats from the plan's threat register:

- **T-04-01** (Information Disclosure on first push) — accept; not yet acted on (push deferred to orchestrator). Threat register's reasoning still holds: repo contains no secrets, only project canon + scaffold + license docs.
- **T-04-02** (Bitwig loading ad-hoc-signed bundle) — confirmed working as intended; ad-hoc signature is the Apple Silicon minimum and Gatekeeper accepts it for `~/Library/Audio/Plug-Ins/` content.
- **T-04-03** (Owner slug in badge) — accepted; substituted to `Aottens`. If the repo moves owners later, badge URL is updated in a routine commit.
- **T-04-04** (First Actions run privilege) — accepted; CI uses default `GITHUB_TOKEN` scopes only.

## Self-Check: PASSED

- `[x]` Task 1 build configure exits 0 — verified (61.5s configure)
- `[x]` Task 1 build compile exits 0 — verified ([100%] Built target Dukko_VST3 + Dukko_CLAP)
- `[x]` Both bundles exist at conventional Pamplejuce paths — `find build -name 'Dukko.vst3' -o -name 'Dukko.clap'` returned both
- `[x]` VST3 binary is arm64 — `lipo -archs ...VST3/Dukko.vst3/Contents/MacOS/Dukko` outputs `arm64`
- `[x]` CLAP binary is arm64 — `lipo -archs ...CLAP/Dukko.clap/Contents/MacOS/Dukko` outputs `arm64`
- `[x]` Both bundles installed to `~/Library/Audio/Plug-Ins/` — `ls -la` confirms both directories exist
- `[x]` Both bundles report a code signature — `codesign -dv` shows `Signature=adhoc` for both
- `[x]` Local pluginval strict-10 PASSED — exit code 0, final line `SUCCESS`
- `[x]` Bundle identifier check — `plutil -p ...Info.plist | grep CFBundleIdentifier` outputs `com.dukkoaudio.dukko` (D-04 baked in correctly)
- `[x]` README.md placeholder substituted — `grep '<your-org>' README.md` returns empty; `grep -E 'github.com/[a-zA-Z0-9_-]+/Dukko/actions' README.md` matches the Aottens URL
- `[x]` Substitution committed — git log shows commit `9f0dc52 chore(01-04): substitute Aottens for <your-org> in README badge URL`
- `[x]` Worktree HEAD safety asserted before commit — branch is `worktree-agent-a80fee6ea35519006`, not a protected ref
- `[x]` No accidental file deletions in commit — `git diff --diff-filter=D --name-only HEAD~1 HEAD` returned empty
- `[x]` STATE.md / ROADMAP.md / REQUIREMENTS.md NOT touched (orchestrator owns these writes) — verified via `git status` clean before SUMMARY write

Commit hash verified against `git log --oneline -5`: `9f0dc52`.
