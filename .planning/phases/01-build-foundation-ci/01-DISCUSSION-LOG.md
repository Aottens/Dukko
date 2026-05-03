# Phase 1: Build foundation & CI - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-05-03
**Phase:** 1-Build foundation & CI
**Areas discussed:** Plugin identifiers, Repo & dir naming, CI scope, chowdsp_utils timing

---

## Plugin identifiers

### Manufacturer name (vendor field in Bitwig browser)

| Option | Description | Selected |
|--------|-------------|----------|
| Aernout Ottens (Recommended) | Personal name — honest for a solo-dev personal-use v1 | |
| Dukko Audio | Brand-it-now — reads as a small label, friendlier for a future commercial release | ✓ |
| Dukko | Plugin-name-as-vendor — simplest, but Bitwig shows 'Dukko / Dukko' which looks redundant | |

**User's choice:** Dukko Audio
**Notes:** User preferred the brand framing despite my recommendation of personal name; signals openness to a future commercial release while not committing to it.

### 4-character manufacturer code (set ONCE for life)

| Option | Description | Selected |
|--------|-------------|----------|
| Aott (Recommended) | Initials of 'Aernout Ottens' — conventional pattern (one uppercase + 3 lowercase) | |
| Dukk | Brand-aligned 4-char | |
| You decide | Pick something else | ✓ |

**User's choice:** You decide → Claude chose `Dukk`
**Notes:** Given D-01 chose `Dukko Audio` as the brand, `Dukk` is the cleanest 4-char vendor abbreviation and reusable for any future Dukko Audio product.

### 4-character plugin code

| Option | Description | Selected |
|--------|-------------|----------|
| Dkk1 (Recommended) | Brand initials + version-family digit | ✓ |
| Duck | Mnemonic, easy to remember in logs | |
| You decide | — | |

**User's choice:** Dkk1

### macOS bundle ID + CLAP plugin ID

| Option | Description | Selected |
|--------|-------------|----------|
| com.dukkoaudio.dukko (Recommended) | Brand-owned namespace | ✓ |
| com.aottens.dukko | Personal namespace | |
| com.github.aernoutottens.dukko | GitHub-username-namespaced | |

**User's choice:** com.dukkoaudio.dukko

---

## Repo & dir naming

### Rename repo + folder?

| Option | Description | Selected |
|--------|-------------|----------|
| Rename now to 'Dukko' (Recommended) | Folder + GitHub renamed; one-time cost, then consistent | ✓ |
| Keep 'KickstartClone' as repo name, only rename CMake target & binaries | Cheaper now but mismatch carried forever | |
| Rename repo to 'dukko' (lowercase) | GitHub convention is lowercase | |

**User's choice:** Rename now to 'Dukko' (capital D)

### Source layout

| Option | Description | Selected |
|--------|-------------|----------|
| Pamplejuce default (source/) (Recommended) | Stick with template | ✓ |
| JUCE convention (Source/) | Capital S, common in old tutorials | |
| Custom (src/ + include/) | Standard CMake split | |

**User's choice:** Pamplejuce default (`source/`)

### Auto-install after build (COPY_PLUGIN_AFTER_BUILD)

| Option | Description | Selected |
|--------|-------------|----------|
| Yes, auto-copy (Recommended) | Tightest dev loop | ✓ |
| No, manual copy | Avoids accidentally clobbering installed Dukko while debugging | |
| Auto-copy only on Release builds | Half-measure | |

**User's choice:** Yes, auto-copy

### GitHub repo creation timing

| Option | Description | Selected |
|--------|-------------|----------|
| Yes, create now, public (Recommended) | Free unlimited macOS-arm64 CI minutes | ✓ |
| Yes, create now, private | 2000 free min/month on `macos-14` for personal accounts | |
| Defer GitHub creation | Phase 1 success criterion #3 can't be verified until later | |

**User's choice:** Public repo now

---

## CI scope

### CI extras beyond floor (multiSelect)

| Option | Description | Selected |
|--------|-------------|----------|
| clap-validator (Recommended) | Validate CLAP target Phase 1 ships | ✓ |
| Steinberg VST3 validator (Recommended) | Catches VST3-spec issues pluginval doesn't | ✓ |
| Upload .vst3 + .clap as workflow artifacts (Recommended) | 30-day retention | ✓ |
| AddressSanitizer build job | Compile-only in Phase 1; no DSP yet | ✓ |

**User's choice:** All four
**Notes:** User pulled Steinberg validator + ASan earlier than the roadmap put them (both were Phase 6). Pattern: wire all quality tooling now so Phase 2+ work lands on infrastructure that already runs.

### CI triggers

| Option | Description | Selected |
|--------|-------------|----------|
| Push to any branch + PRs (Recommended) | Standard Pamplejuce default | ✓ |
| Push to main only | Misses feature-branch regressions | |
| PRs only | Forces PR ceremony for solo dev | |

**User's choice:** Push to any branch + PRs

### Build configurations

| Option | Description | Selected |
|--------|-------------|----------|
| Release only (Recommended) | Validators run against Release; single build per push | ✓ |
| Debug + Release | Doubles build time | |
| Release + Debug-with-ASan | Skips plain Debug | |

**User's choice:** Release only for primary track (with parallel Debug+ASan job from earlier multiSelect)

### CPM cache

| Option | Description | Selected |
|--------|-------------|----------|
| Cache CPM_SOURCE_CACHE between runs (Recommended) | Pamplejuce pattern, ~5min savings | ✓ |
| No cache, fresh fetch every push | Simpler, slower | |

**User's choice:** Cache enabled

---

## chowdsp_utils timing

### When to add chowdsp_utils

| Option | Description | Selected |
|--------|-------------|----------|
| Phase 1 — add CPM block + LICENSES.md entry now (Recommended) | Locks LICENSES.md format from day 1 | ✓ |
| Phase 2 — add when state recall actually needs it | Defers until used | |
| Don't use chowdsp_utils — hand-roll on JUCE primitives | Smaller dep graph, more code to own | |

**User's choice:** Phase 1

### JUCE / clap-juce-extensions pinning

| Option | Description | Selected |
|--------|-------------|----------|
| Pin to specific tags/commits (Recommended) | Reproducible builds | ✓ |
| Track main / latest | Always-latest, breaking-change risk | |
| Pin JUCE, track clap-juce-extensions main | Hybrid | |

**User's choice:** Pin both

### LICENSES.md format

| Option | Description | Selected |
|--------|-------------|----------|
| Hand-curated table (Recommended) | Markdown table, readable, easy to update | ✓ |
| Auto-generated from CMake | Stays in sync, harder to read in PRs | |
| Include full license text per dep verbatim | Required for actual product distribution; overkill for personal v1 | |

**User's choice:** Hand-curated table

### Pamplejuce ingestion

| Option | Description | Selected |
|--------|-------------|----------|
| Use 'Use this template' on GitHub, then commit divergence (Recommended) | Standard pattern | ✓ |
| Fork Pamplejuce, customize, never re-sync | Same as template-clone but loses marker | |
| Copy files manually, no git relationship | Cleanest history but loses audit trail | |

**User's choice:** GitHub "Use this template"

---

## Claude's Discretion

The user delegated specific values where mechanical choice was acceptable:
- 4-char manufacturer code → Claude chose `Dukk` (reasoned from `Dukko Audio` brand)
- Specific JUCE / clap-juce-extensions tags & commits to pin → researcher selects current-latest-stable in plan-phase
- `CMAKE_OSX_DEPLOYMENT_TARGET` value (11.0 vs 12.0) → researcher decides
- CMake version-string scheme — researcher confirms Pamplejuce's `VERSION` macro mechanics, default to semver `0.1.0`
- `.gitignore` content — Pamplejuce default + standard CMake/Xcode build-dir patterns
- README.md vs CLAUDE.md content split — CLAUDE.md exists; README is the GitHub landing page
- Whether to add a CI status badge to README on first push — recommended yes, mechanical decision

## Deferred Ideas

- CI status badge in README — mechanical, fold into Phase 1 plan
- Concurrency `cancel-in-progress` for rapid push cycles — micro-optimization, any later phase
- JUCE / dep version-bump cadence — defer until first drift
- Codesigning / notarization / installer — explicitly out-of-scope per PROJECT.md until commercial release
- Windows runner in CI — v2 milestone, not v1
