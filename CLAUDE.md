# CLAUDE.md — how to work in this repo

Dukko is a tempo-synced volume-ducking plugin (VST3 + CLAP) for Bitwig on Apple Silicon.
Read `PROJECT.md`, `STATE.md`, `ARCHITECTURE.md` and `DECISIONS.md` before doing anything.
Talk to the owner in **Dutch**; write code, comments, specs and commits in **English**.

## The loop: spec → test → code → review

1. **Spec first.** Every feature has a spec in `specs/modules/<module>.md` with numbered
   acceptance criteria (`DSP-02`, `SYNC-01`, …). No code for a criterion that has no number.
   If the spec is missing or unclear, write or extend it and stop for owner approval (gate 1).
2. **Test second.** Each criterion gets a Catch2 test carrying its id in the test name
   (`TEST_CASE ("sync_01_phase_follows_host_ppq")`) in `tests/`. Write the failing test before
   the implementation.
3. **Code third, by a Sonnet subagent.** The main session writes spec and failing tests; a
   Sonnet subagent (`model: sonnet`) writes the smallest change that turns them green; the main
   session reviews the diff before it goes to the owner. Gate: `cmake --build build` with no
   warnings in our code, and `ctest --test-dir build` green.
4. **Review.** One branch and one PR per task on `github.com/Aottens/Dukko`; the owner reviews
   and merges (gate 2). Anything only a DAW can show (loads in Bitwig, state recall, no clicks
   by ear) ships with a checklist in `tests/manual/` that only the owner can tick (gate 3).
5. **Close the loop.** Update `STATE.md`, `BACKLOG.md` and `README.md` in the same PR. The
   README stays a README for users: a new feature goes under *What works today*; `## Status` is
   rewritten, never appended to, at most 8 lines (checked), with no task ids or changelog prose
   (those live in STATE and BACKLOG). A decision made along the way goes in `DECISIONS.md`.
   **Merged = done:** the PR sets its own `BACKLOG.md` row to `done (date, #PR)`, or
   `gate3: <checklist path>` when only the owner's DAW run is left; never `review`/`merged`.
   Open points found in a run go under `## Open points` in the run file
   (`tests/manual/runs/`), each ending in `→` its backlog row, IDEA or checklist, made in that PR.
   `STATE.md` is the handoff (focus, next, gate 3, blocked, owner decisions), not a log.
   `scripts/check-tracking.py` checks this in CI. **Every PR description has a `## Tracking`
   section** with the output of `scripts/check-tracking.py --summary origin/main`.
6. **Clean up after the merge.** In the owner's checkout on `main`:
   `git fetch --prune && git branch -vv | awk '/: gone]/{print $1}' | xargs git branch -D`.

Use the `project-orchestrator` agent in `.claude/agents/` for pre-flight and post-flight checks.

## Sessions (owner rules, carried over from CMDMX)

- **One task per session.** After a task's PR is merged, do not start the next task in the same
  session: update STATE/BACKLOG, then offer the next task as a new session with a
  self-contained brief (task id, criteria, files to read, this loop). Avoids context rot.
- **A session that writes code gets its own worktree** (`use_worktree: true`). Never check out
  or pull in the owner's checkout while another session works there; do the step-6 cleanup only
  when no linked session is working in that folder.
- **Session actions stay in this project:** archive or clean up only sessions whose working
  folder is this repo (or its worktrees). On a bulk action that goes wrong, stop and ask.

## Owner test runs (gate 3)

- **Ask explicitly.** A run the owner must do is its own section in the message ("Test nodig"),
  with numbered steps, what to look or listen for and how to report back; never a path mentioned
  in a recap. It is also the question of the turn.
- **Few steps for the owner.** Tag checklist steps **[Claude]** (setup, building, copying
  plugins, cache resets, repeats, anything a script or computer use can do) or **[Jij]** (the
  5–8 steps that need ears or teach the feature). Walk the owner only through the [Jij] steps.
- **Progress counter.** Every step message shows where the run stands: "Stap 3/5".
- The owner answers per step (ok / fout + what they saw); Claude writes the run file in
  `tests/manual/runs/` with `## Open points`.

## Ideas intake

The owner drops ideas in chat. Record each one in `IDEAS.md` (id, intent, proposed phase,
module, open questions) and ask the owner to confirm the phase. No criteria or code for an idea
until its phase is planned. v1 is `specs/PRD.md`; new ideas default to v2.

## Audio-thread rules (non-negotiable in `processBlock` and everything it calls)

- No allocation, no locks, no syscalls, no logging, no `std::function` construction, no
  exceptions. Data from the UI arrives through atomics or a lock-free snapshot swap.
- `juce::ScopedNoDenormals` at the top of `processBlock`.
- Every audible parameter is smoothed; bypass and curve switches crossfade (DSP-04, CURV-03).
- Tempo-sync phase is derived from the host's PPQ position every block, never accumulated
  (SYNC-01). Guard against `bpm < 1` and missing play head (SYNC-05).
- DSP math lives in `source/dsp/` as plain C++ with no JUCE dependency, so it is testable with
  a fake clock and fake PPQ; the processor is thin glue.

## Plugin identity (never change)

Manufacturer code `Dukk`, plugin code `Dkk1`, bundle and CLAP id `com.dukkoaudio.dukko`, company
"Dukko Audio", product "Dukko". Bitwig projects store these; see `DECISIONS.md`. Parameter ids
are strings and are permanent once released (STAT-01).

## Toolchain notes

- C++20, JUCE 8.0.12, clap-juce-extensions, Catch2 v3, all via `FetchContent` in
  `CMakeLists.txt`. No other build helpers; a new dependency is a decision in `DECISIONS.md`.
- Build: `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build`.
  `COPY_PLUGIN_AFTER_BUILD` installs into `~/Library/Audio/Plug-Ins/{VST3,CLAP}/`.
- Tests: `ctest --test-dir build`. CI also runs pluginval (strictness 10) and clap-validator.
- Host on the owner's Mac: Bitwig Studio (the strict acceptance host).

## Commit style

`<area>: <imperative summary>` — e.g. `dsp: smooth depth and wet/dry (DSP-02, DSP-05)`.
Reference criterion ids in the body. One task per PR.
