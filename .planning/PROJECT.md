# Dukko

> Working repo name: `KickstartClone`. Product/plugin name: **Dukko** (verified May 2026 — no audio-plugin conflicts on KVR / major plugin vendors).

## What This Is

**Dukko** is a VST3 + CLAP audio plugin in the spirit of Cableguys/Nicky Romero **Kickstart** (current shipping version: Kickstart 2) — a tempo-synced volume-ducking effect with editable curves, factory presets, and host sync. Built first for the developer's own use in Bitwig on Apple Silicon, with Windows and a commercial release on the table if v1 turns out well.

## Core Value

**Tempo-locked, click-and-go ducking that sounds and feels good in production** — if everything else fails, dropping it on a track and getting a tight, musical pumping effect must just work.

## Requirements

### Validated

<!-- Shipped and confirmed valuable. -->

(None yet — ship to validate)

### Active

<!-- Current scope. Building toward these. Hypotheses until shipped. -->

#### Core DSP & Behaviour

- [ ] Sample-accurate, tempo-synced volume ducking driven by a curve
- [ ] Curve sync to common note divisions (1/1, 1/2, 1/4, 1/8, 1/16, with dotted/triplet variants)
- [ ] Depth/scale control (0–100% how deep the duck goes)
- [ ] Wet/dry mix
- [ ] Bypass that doesn't click and is host-automatable
- [ ] All parameters automatable + recalled by Bitwig (and by any compliant VST3 host) on project reload

#### Curve System

- [ ] Built-in preset shapes (at minimum the Kickstart 1 family: sine, low pulse, high punch, sidechain-style)
- [ ] User-editable curve (drag points to shape the duck) — at least one custom curve slot per instance
- [ ] Visual curve display that follows playhead

#### UI / UX

- [ ] One-screen UI with curve display, depth, sync division, wet/dry mix, bypass
- [ ] "Strak en bruikbaar" visual style — own visual identity, readable curves, preset dropdown; not showroom-grade yet
- [ ] **Resizable UI at 100% / 125% / 150%** (research: 2026 table-stakes; expensive to retrofit, cheap if planned from day 1)
- [ ] **A/B compare** — toggle between two parameter snapshots (research: cheap once state-snapshot mechanism exists; no competitor in the category has it)
- [ ] No required mouse round-trips for the most common change (preset → depth)

#### Stability / "Bruikbaar voor productie"

- [ ] No audible glitches/clicks under normal use (preset switching, curve edits, automation, bypass toggle)
- [ ] Survives DAW project save/load with all state intact (Bitwig is the strict acceptance host)
- [ ] Host-mediated undo via parameter changes (default for v1; revisit at curve-editor phase)
- [ ] Runs in Bitwig on Apple Silicon as a native arm64 VST3 **and CLAP** (CLAP via `clap-juce-extensions`, Bitwig is CLAP-native)

### Out of Scope

<!-- Explicit boundaries. Includes reasoning to prevent re-adding. -->

- **AU (Audio Unit) build** — developer uses Bitwig; AU only matters for Logic Pro users. Reconsider if commercial release expands target DAW set.
- **AAX (Pro Tools)** — requires Avid Developer license + certification, not worth the overhead for v1.
- **Windows build** — explicit "later" decision; ship Apple Silicon first, port second milestone.
- **Codesigning / notarization / installer** — v1 is for the developer's own machine. Defer until commercial release becomes a concrete decision.
- **Multiband ducking** — category complexity multiplier (3-band crossover, 3 independent curves, mid/side variant); out of scope for v1, candidate for v2.
- **MIDI-trigger mode** — useful but a different sync model than tempo-locked curve; defer to v2 unless trivial to add.
- **Audio-trigger mode / external sidechain input** — entirely separate signal path with transient-detection DSP; defer.
- **Multiple curve slots, randomize, curve import/export, tempo offset** — research-flagged as v1.x candidates; not required for "bruikbaar voor productie" v1.
- **Differentiating "own twist"** — explicitly deferred. Build a strong Kickstart-class base first; the unique angle will surface once the core is in hand.
- **Multi-instance preset sharing / cloud presets** — out of scope for an own-tool v1.
- **Standalone app build** — only the plugin format(s); standalone adds no value when the user is in Bitwig.

## Context

- **User**: Developer themselves, producing in **Bitwig Studio** on **Apple Silicon Mac**.
- **Reference product**: Cableguys/Nicky Romero **Kickstart 3** (https://www.kickstart-plugin.com/).
- **DAW**: Bitwig is VST3-native and also CLAP-native. CLAP is open-source, has no Steinberg license overhead, and could be added cheaply alongside VST3 with most modern frameworks (JUCE, iPlug2). Decision deferred to research.
- **Existing prior art / spike work**: None — greenfield.
- **Skills/learning context**: Not asked, not relevant — Claude builds.
- **Possible commercial future**: If v1 turns out well, likely path is gratis-eerst-of-betaald, requires Windows port, codesigning, installer, presets, and a framework license that allows commercial distribution. This influences early-stage framework choice but does not block v1.

## Constraints

- **Platform (v1)**: Apple Silicon (arm64) macOS only. Native arm64 binary; Rosetta-only is not acceptable.
- **Platform (v2+)**: Windows x64 must be reachable from the same codebase without major rewrites — bias toward cross-platform frameworks from day one.
- **Plugin format (v1)**: VST3 **and CLAP** (CLAP via `clap-juce-extensions`, ~½ day extra work; Bitwig is CLAP-native). AU/AAX explicitly excluded.
- **Host**: Must pass Bitwig's strictness around state recall and parameter automation as the primary acceptance environment.
- **Performance**: CPU footprint should be unnoticeable on a single track at typical project loads (target: <1% CPU per instance on M-series at 48 kHz / 256-sample buffer).
- **License/cost**: Framework choice should keep the door open to a commercial release. Free-for-personal-use frameworks are fine; pure GPL frameworks would force the plugin to be GPL too, which conflicts with "indien heel nice, verkopen".
- **No external services**: Plugin runs fully offline; no telemetry, no license server in v1.

## Key Decisions

<!-- Decisions that constrain future work. Add throughout project lifecycle. -->

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Plugin name: **Dukko** | Distinctive, unclaimed in the audio-plugin space (KVR + major vendors checked May 2026), short, memorable, Dutch-flavoured nod to "duck" | — Pending |
| **VST3 + CLAP for v1** (AU/AAX excluded) | Developer uses Bitwig (CLAP-native); CLAP via `clap-juce-extensions` is ~½ day extra; AU only matters for Logic; AAX requires Avid cert | — Pending |
| Apple Silicon first, Windows second milestone | Developer's machine; cross-platform port is a separate scoped effort | — Pending |
| Reference is current Kickstart (Kickstart 2), v1 = "Kickstart-class base + later own twist" | Avoids over-scoping early; the differentiator will emerge once the core is real | — Pending |
| "Bruikbaar voor productie" is the bar for v1 (not showroom polish, not just "works on my machine") | Aligns scope between basic prototype and commercial-ready | — Pending |
| **Stack: JUCE 8 + CMake/CPM + C++20**, scaffolded from Pamplejuce | Free Personal license up to $50k/yr revenue, mature, VST3+CLAP supported, clear paid upgrade path; matches "commercial future possible, not committed" | — Pending |
| **Curve representation: point + tension model with 1024-sample LUT** | Research-converged; closed-form evaluation, no degenerate states, small data model, audio-thread safe via atomic snapshot pattern | — Pending |
| **Phase derived from host PPQ every block, never accumulated** | Industry-canonical pattern; sample-accurate, self-correcting on loops/scrubs/tempo automation | — Pending |
| **A/B compare and Resizable UI in v1** (research-flagged as cheap if planned early) | A/B is a category gap (no competitor has it); resizable UI is 2026 table-stakes and expensive to retrofit | — Pending |
| Commercial future is *possible*, not committed | Bias framework/architecture toward future commercial use; re-verify JUCE 8 license + sign Steinberg VST3 SDK agreement at the moment of any commercial-release decision | — Pending |

## Evolution

This document evolves at phase transitions and milestone boundaries.

**After each phase transition** (via `/gsd-transition`):
1. Requirements invalidated? → Move to Out of Scope with reason
2. Requirements validated? → Move to Validated with phase reference
3. New requirements emerged? → Add to Active
4. Decisions to log? → Add to Key Decisions
5. "What This Is" still accurate? → Update if drifted

**After each milestone** (via `/gsd-complete-milestone`):
1. Full review of all sections
2. Core Value check — still the right priority?
3. Audit Out of Scope — reasons still valid?
4. Update Context with current state

---
*Last updated: 2026-05-03 after initialization (research integrated, plugin name finalized as Dukko)*
