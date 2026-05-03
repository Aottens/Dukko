# KickstartClone (working title)

## What This Is

A VST3 audio plugin in the spirit of Cableguys/Nicky Romero **Kickstart 3** — a tempo-synced volume-ducking effect with editable curves, presets, and host sync. Built first for the developer's own use in Bitwig on Apple Silicon, with Windows and a commercial release on the table if v1 turns out well.

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

- [ ] One-screen UI with curve display, depth, sync division, mix, bypass
- [ ] "Strak en bruikbaar" visual style — own visual identity, readable curves, preset dropdown; not showroom-grade yet
- [ ] No required mouse round-trips for the most common change (preset → depth)

#### Stability / "Bruikbaar voor productie"

- [ ] No audible glitches/clicks under normal use (preset switching, curve edits, automation)
- [ ] Survives DAW project save/load with all state intact
- [ ] Undo of curve edits inside the plugin (or proper host-mediated undo via parameter changes)
- [ ] Runs in Bitwig on Apple Silicon as a native arm64 VST3

### Out of Scope

<!-- Explicit boundaries. Includes reasoning to prevent re-adding. -->

- **AU (Audio Unit) build** — developer uses Bitwig; AU only matters for Logic Pro users. Reconsider if commercial release expands target DAW set.
- **AAX (Pro Tools)** — requires Avid Developer license + certification, not worth the overhead for v1.
- **Windows build** — explicit "later" decision; ship Apple Silicon first, port second milestone.
- **Codesigning / notarization / installer** — v1 is for the developer's own machine. Defer until commercial release becomes a concrete decision.
- **Multiband ducking** — Kickstart 3 has it, but it's a complexity multiplier; not needed for v1's core value.
- **MIDI-trigger mode** — useful but secondary to the tempo-synced curve; defer to v2 unless trivial to add.
- **Audio sidechain input** — an entire separate signal-path; defer.
- **Differentiating "own twist"** — explicitly deferred. Build a strong Kickstart-3 base first; the unique angle will surface once the core is in hand.
- **Multi-instance preset sharing / cloud presets** — out of scope for an own-tool v1.

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
- **Plugin format (v1)**: VST3. CLAP optional/recommended (Bitwig-native). AU/AAX explicitly excluded.
- **Host**: Must pass Bitwig's strictness around state recall and parameter automation as the primary acceptance environment.
- **Performance**: CPU footprint should be unnoticeable on a single track at typical project loads (target: <1% CPU per instance on M-series at 48 kHz / 256-sample buffer).
- **License/cost**: Framework choice should keep the door open to a commercial release. Free-for-personal-use frameworks are fine; pure GPL frameworks would force the plugin to be GPL too, which conflicts with "indien heel nice, verkopen".
- **No external services**: Plugin runs fully offline; no telemetry, no license server in v1.

## Key Decisions

<!-- Decisions that constrain future work. Add throughout project lifecycle. -->

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| VST3 only for v1 (AU/AAX excluded) | Developer uses Bitwig; AU adds Mac-only Logic surface area, AAX requires Avid certification | — Pending |
| Apple Silicon first, Windows second milestone | Developer's machine; cross-platform port is a separate scoped effort | — Pending |
| Reference product is Kickstart 3, but v1 is "Kickstart 3 base + later own twist" | Avoids over-scoping early; the differentiator will emerge once the core is real | — Pending |
| "Bruikbaar voor productie" is the bar for v1 (not showroom polish, not just "works on my machine") | Aligns scope between basic prototype and commercial-ready | — Pending |
| Commercial future is *possible*, not committed | Bias framework/architecture toward future commercial use, but don't pay v1 cost for v2 features | — Pending |

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
*Last updated: 2026-05-03 after initialization*
