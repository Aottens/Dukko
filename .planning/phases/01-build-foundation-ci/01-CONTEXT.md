# Phase 1: Build foundation & CI - Context

**Gathered:** 2026-05-03
**Status:** Ready for planning

<domain>
## Phase Boundary

Establish the buildable, CI-gated scaffold for **Dukko** — a Pamplejuce-derived JUCE 8 project that builds as a native arm64 VST3 + CLAP bundle on Apple Silicon, loads in Bitwig, and is gated by validators (pluginval strict-10, clap-validator, Steinberg) on every push to GitHub. `LICENSES.md` exists at the repo root from day one. No DSP, no UI, no parameters beyond what the Pamplejuce template ships — Phase 2 owns audio-thread discipline and parameter design.

Greenfield: no source code exists yet. Working repo is currently `KickstartClone`; this phase renames it to `Dukko` and creates the public GitHub repo.

</domain>

<decisions>
## Implementation Decisions

### Plugin Identifiers (PERSISTENT — set once, baked into binaries forever)

- **D-01:** Manufacturer name = `Dukko Audio` (vendor field shown in Bitwig's plugin browser)
- **D-02:** 4-character manufacturer code = `Dukk` (one uppercase + three lowercase per VST3/AU convention; represents the brand, reusable for any future Dukko Audio plugin)
- **D-03:** 4-character plugin code = `Dkk1` (leaves `Dkk2`, `Dkk3` free for future Dukko Audio products)
- **D-04:** macOS bundle ID + CLAP plugin ID = `com.dukkoaudio.dukko` (brand-owned reverse-DNS namespace; same string for both VST3 bundle identifier and CLAP plugin id)

### Repo & Directory Layout

- **D-05:** Rename the working folder from `KickstartClone` → `Dukko` (capital D) AND create the GitHub repo as `Dukko` (capital D) as part of this phase. No git remote exists yet, so this is a clean local rename + fresh GitHub create — no redirect concerns.
- **D-06:** Source layout = Pamplejuce default (`source/` lowercase at repo root). Stay aligned with the template so future template improvements can be cherry-picked cleanly.
- **D-07:** Pamplejuce ingestion method = GitHub "Use this template" then commit divergence. Dukko evolves independently from there; no upstream auto-sync.
- **D-08:** GitHub repo visibility = **public**. Free unlimited `macos-14` Actions minutes on public repos. No commercial release decision yet, so starting public costs nothing and unblocks Phase 1 success criterion #3 (green badge on every push).
- **D-09:** Auto-install after build = ON. Set Pamplejuce's `COPY_PLUGIN_AFTER_BUILD=TRUE` so each local build copies `Dukko.vst3` and `Dukko.clap` into `~/Library/Audio/Plug-Ins/VST3/` and `~/Library/Audio/Plug-Ins/CLAP/` for tightest Bitwig dev loop.

### CI Scope (every push, all branches + PRs)

- **D-10:** **Validators run on every push:**
  - pluginval at strictness level 10 against the Release VST3 (mandatory per QUAL-01)
  - clap-validator (`free-audio/clap-validator`) against the Release CLAP — Phase 1 ships CLAP, so it must be validated from day one rather than waiting for Phase 6
  - Steinberg's official VST3 validator (the `validator` binary bundled in JUCE's VST3 SDK fork) — moved earlier from Phase 6 because wiring cost is trivial and surfaces VST3-spec violations immediately
- **D-11:** Build configurations = **Release** for the validation track + a **parallel Debug+ASan compile-only job**. ASan does not run audio in Phase 1 (no DSP yet) but the tooling is verified working so Phase 2's allocation-guard / audio-thread-safety work lands on a job that already runs.
- **D-12:** Workflow artifacts = upload `Dukko.vst3` and `Dukko.clap` bundles per push (default 30-day retention). Lets you grab a build from another machine without rebuilding and proves the build actually produces both bundles.
- **D-13:** CI triggers = push to any branch + pull requests. Standard Pamplejuce default; catches regressions on feature branches before merge.
- **D-14:** CPM cache = enabled. Cache `CPM_SOURCE_CACHE` between runs (Pamplejuce pattern, `actions/cache` keyed on CMakeLists.txt hash). Cuts CI cold-build from ~6–8 min to ~2–3 min, which matters during Phase 2–3 high-iteration work.

### Dependencies & Versioning

- **D-15:** Pull `chowdsp_utils` via CPM **in Phase 1**, not deferred. Locks the LICENSES.md format from day one (QUAL-05) and means Phase 2's versioned-state work (`chowdsp_plugin_state`) has the dep already present — no CMake change needed mid-phase. Cost: ~30s extra cold-build CPM clone, fully amortized by D-14's cache.
- **D-16:** **Pin JUCE and clap-juce-extensions to specific tags / commit SHAs** (not `main`). Reproducible builds across machines and CI; bump deliberately when breaking-change risk has been considered. Researcher should select the latest stable JUCE 8.0.x tag and a recent green commit on `clap-juce-extensions/main`.
- **D-17:** `LICENSES.md` format = hand-curated markdown table at repo root: `dep | version pin | license | source URL | license-text link`. Tracks JUCE 8, clap-juce-extensions, chowdsp_utils, plus anything else CPM pulls in transitively. Re-verified at every dep bump and at any commercial-release decision.

### Claude's Discretion

The user delegated specific values where mechanical choice was acceptable. Documented above:
- 4-char manufacturer code chosen as `Dukk` (user said "you decide"; reasoned from D-01 Dukko Audio brand)
- Specific JUCE / clap-juce-extensions tags to pin → researcher selects current-latest-stable
- `CMAKE_OSX_DEPLOYMENT_TARGET` → researcher to choose (11.0 is the practical floor for arm64-only macOS Big Sur per CLAUDE.md stack notes; 12.0 is also reasonable)
- CMake version-string scheme (semver `0.1.0` recommended; researcher confirms Pamplejuce's `VERSION` macro mechanics)
- `.gitignore` content (Pamplejuce default + standard CMake/Xcode build dirs)
- README.md vs CLAUDE.md content split (CLAUDE.md already exists; README is for the GitHub landing page)
- Whether to add a CI status badge to README on first push

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Project canon (read every phase)
- `.planning/PROJECT.md` — Product definition, constraints, key decisions, out-of-scope
- `.planning/REQUIREMENTS.md` — v1 requirement IDs (BUILD-01..05, QUAL-01, QUAL-05 are this phase's scope)
- `.planning/ROADMAP.md` §"Phase 1: Build foundation & CI" — goal, depends-on, success criteria
- `.planning/STATE.md` — Accumulated decisions table (already includes D-15-equivalent stack lock)
- `CLAUDE.md` §"Technology Stack" through §"Stack Patterns by Variant" — full stack rationale, alternatives ruled out, version-compatibility tables

### Research artifacts (stack & architecture canon)
- `.planning/research/STACK.md` (referenced from STATE.md and CLAUDE.md) — JUCE 8 + CMake/CPM + C++20 + Pamplejuce decisions
- `.planning/research/ARCHITECTURE.md` (referenced from STATE.md) — atomic CurveSnapshot, PPQ-derived phase, XML state schema (Phase 2+ relevance, but useful to keep top-of-mind for non-divergent scaffolding)
- `.planning/research/PITFALLS.md` (referenced from STATE.md) — audio-thread discipline pitfalls (Phase 2 owner; Phase 1 must not block them)
- `.planning/research/SUMMARY.md` (referenced from STATE.md) — research convergence and rationale

### External / upstream (no relative path — link form)
- Pamplejuce template: https://github.com/sudara/pamplejuce — phase-1 scaffolding base
- JUCE 8: https://github.com/juce-framework/JUCE/releases — pin to latest stable 8.0.x tag
- clap-juce-extensions: https://github.com/free-audio/clap-juce-extensions — pin to a green commit on `main`
- chowdsp_utils: https://github.com/Chowdhury-DSP/chowdsp_utils — CPM block in Phase 1
- pluginval: https://github.com/Tracktion/pluginval/releases — version 1.0.4
- clap-validator: https://github.com/free-audio/clap-validator/releases — latest
- JUCE 8 EULA (license verification): https://juce.com/legal/juce-8-licence/

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- **None** — Phase 1 is greenfield. The repo currently contains only `.planning/` and `CLAUDE.md`. Pamplejuce's template files become the starting point.

### Established Patterns
- **CLAUDE.md style** — written in the existing project's voice ("strak en bruikbaar", short Dutch phrases for emphasis, table-driven decisions). Phase 1's README and any new docs should match.
- **Documentation conventions** — `.planning/` artifacts use sentence-case headings, short rationale prose, decision tables. Phase 1 docs (LICENSES.md, README.md) should follow.

### Integration Points
- **`.planning/STATE.md` will need a new entry** at phase completion logging the actual JUCE / clap-juce-extensions / chowdsp_utils version pins chosen.
- **GitHub repo `Dukko` is created during this phase** — every subsequent phase assumes it exists and CI is green.
- Phase 2 (DSP scaffold & state recall) inherits the entire CMake graph, plugin identifiers, and CI workflow from Phase 1 untouched. Identifier changes after Phase 1 ship would break state recall in any Bitwig project that ever loaded a built plugin — that's the correctness consequence of getting D-01..D-04 right NOW.

</code_context>

<specifics>
## Specific Ideas

- The user prefers **public GitHub repo** despite no committed commercial future — minimal cost, maximum CI capacity (D-08).
- The user opted into the **full validator + ASan matrix from day one** even where roadmap deferred it (Steinberg validator was Phase 6, ASan was Phase 6) — preference is clearly "wire all quality tooling now even if it's idle in Phase 1, so Phase 2+ work lands on infrastructure that already runs" (D-10, D-11).
- The user picked recommended options on every architectural/identifier question and only delegated when a recommendation was already strong — discussion was efficient. Default toward reasoned recommendations in subsequent phase discussions.

</specifics>

<deferred>
## Deferred Ideas

- **CI status badge in README** — mechanical; researcher / planner can include in Phase 1 plan as a one-line task without re-asking.
- **Concurrency `cancel-in-progress` for rapid push cycles** — micro-optimization; can be added in any later phase that touches CI.
- **JUCE / dependency version-bump cadence** — defer until first version actually drifts; not a Phase 1 concern.
- **Codesigning, notarization, installer** — explicitly out-of-scope per PROJECT.md until commercial release decision (which is itself out of scope for v1).
- **Windows runner in CI** — Windows is the v2 milestone per PROJECT.md; do not add `windows-latest` to the matrix in v1.

</deferred>

---

*Phase: 1-Build foundation & CI*
*Context gathered: 2026-05-03*
