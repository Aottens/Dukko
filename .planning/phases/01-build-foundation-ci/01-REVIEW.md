---
phase: 01-build-foundation-ci
reviewed: 2026-05-03T00:00:00Z
depth: standard
files_reviewed: 6
files_reviewed_list:
  - .github/workflows/build_and_test.yml
  - CMakeLists.txt
  - LICENSES.md
  - README.md
  - source/PluginProcessor.cpp
  - source/PluginProcessor.h
findings:
  critical: 0
  warning: 6
  info: 5
  total: 11
status: issues_found
---

# Phase 1: Code Review Report

**Reviewed:** 2026-05-03
**Depth:** standard
**Files Reviewed:** 6
**Status:** issues_found

## Summary

Phase 1 ships scaffolding (Pamplejuce-derived) plus four authored deltas: the
CI workflow, the Dukko-specific CMakeLists customizations, LICENSES.md, the
README, and a small round-trippable-state fix in PluginProcessor.cpp. The
authored work is sound in the broad strokes — CI is green end-to-end, identifier
invariants are enforced, license tracking is set up correctly — but there are a
handful of fragility and hygiene issues that should be cleaned up before they
calcify into "the way we do things here."

No security blockers, no correctness blockers. Six warnings (mostly
fragility / fail-open behavior in the CI workflow and a stale README claim) and
five info items.

## Warnings

### WR-01: LICENSES.md staleness guard uses substring grep — false positives mask real misses

**File:** `.github/workflows/build_and_test.yml:135`
**Issue:** The guard checks for a dep with `grep -qi "$dep_name" LICENSES.md`.
This is a case-insensitive *substring/regex* match against the entire file,
including prose. Two failure modes follow:

1. **False positive:** A future contributor removes the `JUCE` row from the
   table but leaves the prose mention in the GPLv3 note ("pluginval is licensed
   GPLv3..." — which doesn't mention JUCE, but the prose section about
   "JUCE 8 SDK fork" does mention it). The guard passes; the table is wrong.
2. **Regex injection:** `dep_name` is interpolated into a regex unquoted. If a
   future dep is named with regex metacharacters (e.g., `lib++`, `c.utils`),
   the match is undefined. Currently safe (`juce`, `clap-juce-extensions`,
   `chowdsp_utils`, `melatonin_inspector` — all alnum + `-`/`_`) but fragile.

**Fix:** Match against the actual table column. Either parse the markdown table,
or use a more anchored grep against the leftmost cell:
```bash
if ! grep -qE "^\| *${dep_name}( |\|)" LICENSES.md; then
  echo "MISSING in LICENSES.md table: $dep_name"
  exit 1
fi
```
For full safety, `grep -F` (fixed-string) plus an explicit anchor on the table
row format eliminates both the prose-match and regex-injection issues.

### WR-02: GitHub Actions workflow grants default (writable) permissions

**File:** `.github/workflows/build_and_test.yml:27` (top-level — missing block)
**Issue:** No `permissions:` block is declared. On pushes to branches in the
repo (not PRs from forks), the default `GITHUB_TOKEN` is granted write access
to contents, issues, pull-requests, statuses, packages, etc. This is a
build-and-validate workflow that does not need to write anything to the repo
or to any GitHub API surface. Principle of least privilege applies; if any
third-party action ever gets compromised (or one of the third-party tarball
downloads is swapped — see WR-04), a writable token amplifies the blast
radius (e.g., the action could push to `main` or open PRs).

**Fix:** Add at the top of the workflow:
```yaml
permissions:
  contents: read
```

### WR-03: README claims the tree is not buildable — stale on commit 5ff5141

**File:** `README.md:29-31`
**Issue:** The README says "Plan 01 ships the rename / scaffold pass only —
the tree is not yet buildable end-to-end. Plan 02 wires up JUCE 8 +
clap-juce-extensions + chowdsp_utils via CPM and is what makes `cmake -B build`
actually succeed." This was true at the start of Phase 1 but is no longer
true after Plan 04: CI is green end-to-end, all three CPM blocks are in
`CMakeLists.txt`, and `cmake -B build && cmake --build build` produces both
VST3 and CLAP bundles. Anyone reading the README before opening
`CMakeLists.txt` will be misled.

**Fix:** Replace the paragraph with a current-state description, e.g.:
```markdown
Phase 1 ships the build foundation + CI; the tree builds end-to-end on
macOS arm64 and produces both VST3 and CLAP bundles, but there is no DSP
yet — the plugin is silent. Phase 2 wires up parameters, state, and the
ducker DSP.
```

### WR-04: Validator binaries downloaded over HTTPS without integrity check

**File:** `.github/workflows/build_and_test.yml:88-93, 109-114`
**Issue:** Both `pluginval_macOS.zip` and
`clap-validator-0.3.2-macos-universal.tar.gz` are downloaded with
`curl -L <url>` and immediately extracted/executed. There is no SHA256
verification. A compromise of the GitHub Releases CDN, an MITM on a
misconfigured runner, or a malicious release re-tag would inject arbitrary
code into the runner — and that runner has the workflow's `GITHUB_TOKEN`
(which today is writable, see WR-02). For pinned-version downloads in CI
this is a one-line fix and a meaningful supply-chain hardening.

**Fix:** Pin the SHA256 of each release asset and verify after download:
```bash
echo "<known-sha256>  pluginval.zip" | shasum -a 256 -c -
```
Capture each pin once when bumping the version (the same moment the URL
is updated) and check it in alongside the URL change. Same pattern for
clap-validator.

### WR-05: `#if (MSVC)` is never true — IPP include is dead code, and the gate is wrong

**File:** `source/PluginProcessor.h:5`
**Issue:** `MSVC` is not a predefined preprocessor macro. The intended
Windows/MSVC sentinel is `_MSC_VER`. As written, `#if (MSVC)` is always
false (MSVC expands to nothing, evaluating to `#if ()` which the
preprocessor treats as 0), so `ipps.h` is never included via this guard.
This is harmless on Phase 1's macOS-only target, but it is a latent bug
that will quietly break IPP usage when Windows enters scope (Phase 6 per
ROADMAP) — anyone porting to Windows will hit "unresolved IPP symbols"
with no obvious cause because the include site looks correct.

**Fix:**
```cpp
#if defined(_MSC_VER)
#include "ipps.h"
#endif
```
Or, better, gate on a project-defined `DUKKO_HAS_IPP` symbol set by the
`DukkoIPP` CMake helper so it's not silently disabled when the helper
decides IPP isn't available.

### WR-06: `CMAKE_OSX_ARCHITECTURES` is `FORCE`-set — silently ignores user override

**File:** `CMakeLists.txt:6-7`
**Issue:** `set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "" FORCE)` and
the same pattern for `CMAKE_OSX_DEPLOYMENT_TARGET`. The `FORCE` flag means
that a future contributor running e.g.
`cmake -B build -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"` to produce a
Universal binary will have their override silently overwritten with `arm64`
during the configure step. The comment correctly notes this is a v1 lock,
but `FORCE` is the wrong mechanism — it confuses the user (their flag
appears to do nothing, with no warning). The CI workflow already passes
`-DCMAKE_OSX_ARCHITECTURES=arm64` explicitly, so `FORCE` buys nothing in CI.

**Fix:** Drop `FORCE` so user overrides work, and have CI continue passing the
flag explicitly (it already does):
```cmake
set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Build architectures")
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
```
If the intent is genuinely to forbid Universal builds in v1, fail explicitly:
```cmake
if (NOT CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
    message(FATAL_ERROR "Phase 1 ships arm64 only; see PROJECT.md")
endif ()
```

## Info

### IN-01: Workflow checks out submodules recursively, but the project has none

**File:** `.github/workflows/build_and_test.yml:53-54, 169-170`
**Issue:** Both jobs use `submodules: recursive` on checkout. `LICENSES.md`
explicitly states "no submodule / no upstream tracking" — Pamplejuce was
ingested by copy. Recursive submodule checkout is a no-op here, just adds a
small amount of CI time and one more piece of misleading config for future
readers.

**Fix:** Remove the `with: submodules: recursive` block from both jobs.

### IN-02: No `concurrency:` group — duplicate runs on rapid pushes consume runner minutes

**File:** `.github/workflows/build_and_test.yml:29-33`
**Issue:** Triggers fire on every push to every branch and every PR. Two
quick pushes to the same branch run two full builds in parallel; the older
one is wasted work. macos-14 runners are paid minutes (or quota) — easy
saving, no behavior change.

**Fix:**
```yaml
concurrency:
  group: ${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: true
```

### IN-03: `actions/*` pinned to major-version tags rather than commit SHAs

**File:** `.github/workflows/build_and_test.yml:52, 57, 143, 152`
**Issue:** `actions/checkout@v4`, `actions/cache@v4`,
`actions/upload-artifact@v4` are pinned to floating major tags. GitHub's own
hardening guide recommends commit-SHA pinning for third-party actions; for
first-party `actions/*` it's lower risk but still a defense-in-depth
improvement (a compromise of the action's repository would let a malicious
maintainer move the `v4` tag to a compromised commit). Lowest priority of
the security findings — listed for completeness.

**Fix:** Pin to commit SHAs with a comment naming the tag:
```yaml
uses: actions/checkout@b4ffde65f46336ab88eb53be808477a3936bae11 # v4.1.1
```

### IN-04: `setStateInformation` silently accepts garbage / empty data

**File:** `source/PluginProcessor.cpp:179-184`
**Issue:** The current Phase 1 implementation reads a `ValueTree` from the
input stream and discards it (`juce::ignoreUnused (state)`). It does not
verify that the parsed tree has type `"DukkoState"`, does not check whether
parsing produced an invalid `ValueTree`, and does not handle `sizeInBytes
== 0`. For Phase 1's "round-trip nothing" stub this is acceptable — there
is no state to corrupt — but the comment claims this is "minimum-viable
round-trippable state" while the only thing being verified is that
`load()` doesn't crash on the bytes `getStateInformation` produces.
Phase 2's `chowdsp_plugin_state` migration will replace this; flagging only
so the assumption "round-trip" is documented to mean "load() returns true,
not bit-equal state."

**Fix (optional, Phase 1 scope):** assert the type so a future regression in
`getStateInformation` is caught at the next CI run:
```cpp
auto state = juce::ValueTree::readFromStream (stream);
jassert (state.hasType ("DukkoState"));
juce::ignoreUnused (state);
```

### IN-05: `clap_juce_extensions_plugin` hardcodes the CLAP_ID literal instead of `${BUNDLE_ID}`

**File:** `CMakeLists.txt:174-178`
**Issue:** `CLAP_ID "com.dukkoaudio.dukko"` is a literal duplicate of
`BUNDLE_ID` set on line 52. The comment explains this is intentional ("so the
plan-02 grep gates match and so a future reader sees the canonical CLAP
plugin id at the wiring site without chasing variables"). Trade-off accepted,
but D-04 invariant ("CLAP_ID == BUNDLE_ID") is now enforced only by code
review and grep gates, not by the build system. If Phase 1's grep gate is
ever retired, the duplication becomes drift-prone.

**Fix (optional):** Use the variable and add a CMake `if` that asserts the
literal matches, getting the best of both:
```cmake
if (NOT BUNDLE_ID STREQUAL "com.dukkoaudio.dukko")
    message(FATAL_ERROR "D-04 invariant broken: BUNDLE_ID changed")
endif ()
clap_juce_extensions_plugin(
    TARGET Dukko
    CLAP_ID "${BUNDLE_ID}"
    CLAP_FEATURES audio-effect mixing
)
```

---

_Reviewed: 2026-05-03_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
