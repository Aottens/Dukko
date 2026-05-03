# Feature Research

**Domain:** Tempo-synced volume-ducking VST3 audio effect (Kickstart-class plugin)
**Researched:** 2026-05-03
**Confidence:** MEDIUM-HIGH

> **Naming note:** The PROJECT.md / brief refers to "Kickstart 3", but as of May 2026 the current shipping product on kickstart-plugin.com is still **Kickstart 2** (Cableguys/Nicky Romero). All references below to "Kickstart" mean the current Kickstart 2 product, which is what the brief intends as the reference ceiling. Kickstart 1 (the original 2014 product) is the bare-bones predecessor.

---

## Reference Product Survey

Quick positioning of the four reference products in this category. This anchors the rest of the analysis.

| Product | Price (2026) | Position | Curve model | Sync | Trigger modes | Multiband | Notable |
|---------|--------------|----------|-------------|------|---------------|-----------|---------|
| **Kickstart 2** (Cableguys/Nicky Romero) | ~$16 / €14 | Cheapest, fastest, "click-and-go" | 16 hand-crafted curves + draggable slope (limited reshaping, not free-form) | 1/8, 1/4, 1/2, 1/1 only | Tempo sync, MIDI trigger, audio trigger (auto-follow non-4x4 kicks) | Yes — band-split mode (duck only lows) | Resizable UI 75-200%; no full free curve editor; loop + one-shot modes |
| **VolumeShaper 6** (Cableguys, also in ShaperBox 3) | $34 standalone, ~$99-129 in ShaperBox 3 bundle | Pro flexible LFO/volume tool | Free-form curve, up to 40 nodes per LFO, snap-to-grid + shift fine-tune | Sample-accurate, broad division set, multi-bar lengths | Tempo sync, MIDI trigger, audio transient trigger, external sidechain (with kick overlay visualization) | Yes — 3 bands (low/mid/high), independent LFO each, user-set crossover frequencies | "Show External Sidechain" overlay; user preset save/load; pro feature ceiling |
| **Duck 2** (Devious Machines) | ~$40 / €35 | Ducking-specialist with audio-aware tools | Multi-segment envelopes with per-point curve shapes, paintbrush tools, shape-from-wave import, dynamic swing | Tempo sync (Lock to Beat), flexible grid | MIDI, sidechain audio, audio (input transient), Repeat (free-running) | Yes — up to 3 bands, linear-phase filtering | Trace Envelope (extracts envelope from sidechain audio); Offset+Lookahead unified continuous control; instance-sync groups (up to 4); randomize |
| **LFOTool** (Xfer Records) | ~$49 | Generic tempo-synced LFO router (not ducking-specialist) | Point + tension-curve editor, 12 graphs per preset (switchable via MIDI / automation), up to 4 graphs simultaneously (cutoff/reso/pan + amp) | Sample-accurate BPM sync (with optional swing) or Hz | Tempo sync, MIDI note triggers graph switching, MIDI CC out | No (single-band) | Outputs MIDI CC to control other instruments; classic free-form curve drawing tool |

**Where a Kickstart-class clone sits:** The cheap-and-fast end of this market. Kickstart 2 at $16 deliberately undercuts VolumeShaper/Duck/LFOTool by being *less flexible on purpose* — the value is "drop on track, get good ducking, done". A clone targeting this tier should resist creeping into VolumeShaper territory or it loses its identity (and its price floor).

---

## Feature Landscape

### Table Stakes (Users Expect These)

Missing any of these in a 2026 ducking plugin = users assume it's broken or unfinished. Every product surveyed has all of these.

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| **Tempo sync to host BPM** | The entire category is "tempo-synced ducking". Without host BPM, the product doesn't exist. | S | VST3 host provides `processContext.tempo` and PPQ position per process block. Pure read; no clock recovery needed. |
| **Multiple sync divisions (at minimum 1/1, 1/2, 1/4, 1/8)** | Even Kickstart 2 — the most stripped-down product — has these four. | S | Discrete enum parameter. |
| **Fine sync divisions (1/16, dotted, triplet)** | VolumeShaper, Duck, LFOTool all have these; Kickstart 2 conspicuously *doesn't*. Users in trap/DnB/half-time genres expect 1/16 + triplet. | S | Just more enum values + math. The constraint is UI room, not DSP. |
| **Depth / amount knob (0-100% how deep the duck goes)** | Universal. Often called "Mix" on Kickstart. | S | Linear interpolation between unity and curve value. |
| **Wet/Dry mix** | Universal in modulation effects. Lets users blend processed against original. | S | Output-stage crossfade. Note: Kickstart's "Mix" knob conflates depth and wet/dry; cleaner products separate them. |
| **Click-free bypass (host-automatable)** | VST3 hosts (and Bitwig especially) expect bypass to be a parameter on the plugin, ramped internally to avoid clicks. | S-M | VST3 has a dedicated `kIsBypass` parameter flag. Internal short ramp (e.g. 5-10ms) on bypass change. |
| **Click-free preset/curve switching** | Users will A/B presets during playback. Audible clicks/discontinuities = unusable. | M | Need to crossfade between curve sources or smooth gain at the moment of switch. Easy to overlook in v1. |
| **Full parameter automation** | Bitwig/Ableton/Logic users automate everything. Plugin parameters not exposed = "broken". | S | Every user-facing control must be a VST3 parameter, not a hidden internal state. |
| **Host state recall (project save/load)** | If the plugin doesn't restore exactly on project reload, users lose work. Non-negotiable. | M | VST3 `IComponent::getState`/`setState`. Need versioned format for forward compat. Curve points must persist, not just parameter IDs. |
| **Visual curve display** | Every product surveyed has this. Without it, users can't see what the duck is doing. | M | Static draw of curve + dynamic playhead overlay. |
| **Playhead indicator on curve** | Universal. Lets the user see *where in the duck* the audio currently is. | S | Read PPQ position, modulo curve length, draw a vertical line. |
| **Built-in factory curves/presets** | Kickstart's whole identity is "16 curves Nicky uses". VolumeShaper, Duck, LFOTool all ship factory presets. | S | Static data baked into the binary. The PROJECT.md already calls for the Kickstart 1 family (sine, low pulse, high punch, sidechain). |
| **Resizable UI** | Modern expectation (2020+). Kickstart 2 explicitly added 75-200%. Bitwig has retina displays where fixed-size UIs look microscopic. | M | JUCE/iPlug2 both support this; need to design UI in resolution-independent units. |
| **Undo/redo of curve edits** | Every product with editable curves has this. Drawing curves is exploratory; without undo, users won't dare experiment. | M | Either in-plugin undo stack OR strict reliance on host-mediated undo via parameter changes. PROJECT.md acknowledges either is acceptable for v1. |

### Differentiators (Competitive Advantage)

These are where products in this category compete. Not all products have all of these. The "Kickstart 3-niveau clone" target implies hitting *most* of these but not all (Kickstart deliberately doesn't have a free-form curve editor — that's VolumeShaper's territory).

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| **Free-form editable curve with bezier/tension handles** | The single biggest UX leap from Kickstart 1 → Kickstart 2 / VolumeShaper. Lets users dial in *exactly* the duck shape they want. PROJECT.md already requires "at least one custom curve slot". | L | Several sub-decisions: (a) point + tension (Xfer style — curve between two points has a tension parameter) vs (b) bezier control handles vs (c) per-segment shape enum (Duck 2 style). Tension model is simplest and most musical. Has implications for state serialization, UI hit-testing, audio-rate evaluation, and sample-accurate sync. |
| **Multiple curve slots / preset bank** | LFOTool: 12 graphs per preset, switchable via MIDI. Duck 2: envelope presets. Lets users keep curves for verse/chorus/drop in one instance. | M | Needs per-slot state and a switching mechanism (UI + automation parameter + optionally MIDI). Cheap if curves are small data. |
| **MIDI-trigger mode (volume duck retriggered by note-on)** | Industry standard for non-4-on-the-floor genres (trap, hip-hop, DnB). Kickstart 2 has it; PROJECT.md currently defers to v2. | M | Different sync model: instead of `position = PPQ % curveLength`, position resets on MIDI note-on. Requires VST3 event input, retrigger logic, optional one-shot vs loop. **Conflicts** with pure tempo-sync mode — needs a mode switch. |
| **Audio-trigger / transient-following** | Kickstart 2 markets this as the headline feature ("works on any kick pattern, not just 4-on-the-floor"). Same value proposition as MIDI trigger but driven by the audio itself. | L | Requires transient detection DSP (envelope follower + threshold, or a proper onset detector). Easy to do badly; getting it musical is the hard part. Defines product feel. |
| **External sidechain audio input** | VolumeShaper, Duck use this. Lets users trigger the duck from a different track than the one being processed. | M | VST3 supports a second audio input bus (sidechain bus). Need to wire it through the processor and expose UI for source selection (main vs sidechain). The DSP is the same as audio-trigger; the routing is the difference. |
| **Multiband processing (split into 2-3 bands, duck per band)** | Kickstart 2, VolumeShaper, Duck 2 all have it. Used for "duck only the lows" — the bass is ducked but the cymbals/synths aren't. The most-requested feature on top of basic ducking. PROJECT.md explicitly excludes it from v1. | L | Requires crossover filters (Linkwitz-Riley 4th order is standard) + per-band gain stage + UI for crossover frequencies. Linear-phase filtering (Duck 2) adds latency and complexity but avoids phase smearing — pro tier feature. |
| **Retrigger / phase reset on note start (MIDI or audio)** | Coupled with MIDI-trigger or audio-trigger modes. Without retrigger, the curve wraps continuously regardless of what triggered it. | S (given trigger mode) | Trivial DSP; conceptually requires a "reset playhead to 0" event in the curve evaluator. |
| **Swing / groove offset on the curve** | LFOTool has "optional swing"; Duck 2 has "dynamic swing". Lets the duck breathe in non-straight rhythms. | M | Time-warp the curve evaluation: every other cycle gets shifted by `swing%`. Sounds simple but needs care to avoid discontinuities at the boundaries. |
| **Tempo offset / push-pull timing** | Duck 2 unifies offset and look-ahead into "continuously variable time offset". Lets users push the duck a few ms early or late to align with snare/kick attack. Subtle but pro-feel. | S | Just a delay line on the curve evaluation, can be negative (look-ahead) or positive (delay). Look-ahead requires a buffer of dry audio. |
| **Look-ahead** | Duck 2 has this for "glitch-free audio" when responding to transients. Distinct from look-ahead-as-offset; this is "delay the dry signal so the curve can react in time". | M | Adds latency (host-reported via VST3 latency API). Necessary for transient-driven modes; useless in pure tempo-sync. |
| **Mid/side or side-only processing** | Niche but appreciated — duck only the sides for stereo width pumping without affecting mono center. Not in any of the surveyed products in a headline way; would be a mild differentiator. | M | Encode L/R → M/S, gain-modulate the side channel only, decode back. Self-contained block. |
| **Oversampling** | LFOTool, Duck 2 mention pro audio quality. Volume modulation at high rates can produce aliasing in the sidebands; oversampling reduces that. | M | 2x or 4x polyphase or windowed-sinc upsample → process → downsample. CPU cost scales linearly with oversampling factor. Often runtime-toggleable. |
| **A/B compare** | Standard pro-plugin feature. Lets users hold two parameter states and toggle between them. | S | Two copies of full plugin state in memory; A/B button just swaps which one drives the audio. Surprisingly cheap. |
| **Randomize curve** | Duck 2 has "multi-mode randomise"; Kickstart doesn't. Generates new shapes when stuck. Creative tool. | S | Randomize point positions/tensions within musical constraints (e.g. snap to grid, smooth). |
| **Audio-meter / oscilloscope view** | Kickstart 2's "kick view" overlays kick + bass; Duck 2 has metering. Helps users see what's actually being ducked. | M | Just a visualizer over input/output buffers. Pure UI. |
| **Curve import/export (share between projects/users)** | Implied by Kickstart 2's preset system and Duck 2's "shape import from wave files". Required for v2 if there's a community angle. | S-M | Curves as JSON or proprietary binary. Drag-drop file or copy-paste base64. PROJECT.md explicitly excludes "cloud presets" but not local file save/load. |
| **CLAP plugin format alongside VST3** | Bitwig is CLAP-native; CLAP is open-source with no Steinberg license. Could be added cheaply with JUCE7+/iPlug2 next to VST3. | S-M | A wrapper rebuild, not a rewrite of DSP. Worth flagging because the developer's own DAW (Bitwig) is the strongest CLAP host. |
| **Instance synchronization (multiple plugin instances stay in lockstep)** | Duck 2 has "up to 4 groups" for cross-instance sync. Niche but useful for layered processing. | M | Shared static state (group ID → phase). Subtle bugs around instance lifecycle; not v1 material. |

### Anti-Features (Commonly Requested, Often Problematic)

These are features that *seem* like obvious additions but would either bloat scope, dilute the core value (tempo-locked, click-and-go ducking), or push the product into the territory of a more general tool (where it would lose to VolumeShaper/Duck/LFOTool anyway).

| Feature | Why Requested | Why Problematic | Alternative |
|---------|---------------|-----------------|-------------|
| **Full sidechain compressor with attack/release/threshold/ratio/knee** | "If it's already ducking volume, why not just be a compressor?" | Completely different DSP and UX paradigm. Compressor users want gain reduction meters, ratio knobs, RMS/peak detection modes. Adding it makes the plugin worse at *both* jobs and competes with FabFilter Pro-C / TDR Kotelnikov / native DAW compressors. | Stay tempo-curve-driven. Users wanting a compressor have 50 better options. |
| **Full multiband EQ / surgical filter section** | "I'm already splitting into bands; why not add EQ per band?" | Enormous scope creep. You're now an EQ plugin. Filter design, GUI for frequency/Q/gain, phase considerations. Pulls focus from the curve experience. | If multiband ducking is added (v2+), expose only crossover frequencies, not full per-band EQ. |
| **Parallel compression / mix-to-original blending beyond wet/dry** | "Pro mixers use parallel comp; let me dial in -X dB of original alongside processed." | Wet/dry already covers this. Adding a separate "parallel" knob with its own dB scale invites confusion (which knob is the user supposed to turn?). | One wet/dry knob. Document that 50% is parallel-compression-equivalent. |
| **DAW-style modulation matrix (LFO modulating depth, etc.)** | LFOTool / FilterShaper users like modulating the modulator. | Kickstart 2's identity is "click and go". A modulation matrix turns it into a soft synth. Users wanting this should buy LFOTool/FilterShaper XL. | None — explicitly out of scope. The tempo curve *is* the modulator. |
| **MIDI generation / output (like LFOTool's CC out)** | LFOTool can output CC to drive other plugins. | This is a *generator* role, not an *effect* role. VST3 effect plugins outputting MIDI is supported but unusual; Bitwig handles it but the workflow is different. Adding it means designing a routing UI and dealing with MIDI CC scaling. | Out of scope. If users want LFO-as-modulator, LFOTool exists. |
| **Built-in saturation / distortion / "color" stages** | "Add some warmth" feature creep. | Distortion choices are taste-dependent and divisive. Every user wants a different color. Adds CPU, adds parameters, dilutes "ducking plugin" identity. | Users add distortion as a separate plugin in their chain. |
| **Built-in reverb send / "duck the reverb tail" feature** | "I always sidechain my reverb anyway, build it in." | Reverb design is its own product category. Plus the user can already insert this plugin *after* a reverb. | Document the workflow ("place after reverb send"). Don't build a reverb. |
| **Cloud preset sharing / community preset browser** | Modern plugins increasingly do this (Splice, native cloud features). | Backend infrastructure, accounts, moderation, hosting costs. Useless for a v1 own-tool. PROJECT.md explicitly excludes it. | Local file import/export of curves is a perfectly fine alternative for v2. |
| **AI / machine-learning curve generation** | 2024-2026 trendy. | Massive complexity, tiny user benefit for a curve that takes 10 seconds to draw. Onnx model in plugin = bloat. | Randomize button covers 80% of "give me ideas" use case at 1% of the complexity. |
| **Per-instance metering with LUFS / peak / dBFS readouts** | Pro mixing engineers want metering. | Metering is a deep rabbit hole (true peak, K-weighting, integration time). Users have dedicated metering plugins. | Show input/output level as simple peak meters in the UI; don't pretend to be a mastering meter. |
| **Stand-alone application (run without a DAW)** | JUCE makes it cheap to add. | Nobody uses a tempo-synced ducking effect outside a DAW. The whole value depends on host BPM. | Pure plugin, no standalone. |
| **Modulation of every parameter via host automation lanes "drawn into" the plugin** | "Bitwig modulators can already do this, why does the plugin need it?" | Reinventing what the host already does, badly. | Make sure all parameters are properly automatable in the host. Let the host do its job. |
| **Look-ahead in tempo-sync mode** | "Why isn't there look-ahead?" — users assume it's universally good. | Look-ahead only matters when reacting to a *signal*. In pure tempo-sync mode, the curve position is known infinitely far in advance — look-ahead is meaningless and just adds latency. | Only enable look-ahead in audio-trigger mode (if/when that ships). |

---

## Feature Dependencies

```
Tempo sync to host BPM (foundation)
    │
    ├──> Sync divisions (1/1, 1/2, 1/4, 1/8, 1/16, dotted, triplet)
    │       │
    │       └──> Curve playback engine (PPQ-driven evaluator)
    │               │
    │               ├──> Visual curve display
    │               │       │
    │               │       └──> Playhead indicator
    │               │
    │               ├──> Built-in factory curves (data only)
    │               │
    │               ├──> Free-form curve editor
    │               │       │
    │               │       ├──> Multiple curve slots / preset bank
    │               │       │
    │               │       ├──> Randomize curve
    │               │       │
    │               │       ├──> Curve import/export
    │               │       │
    │               │       └──> Undo/redo of curve edits
    │               │
    │               └──> Click-free curve switching
    │
    ├──> Depth knob ──> Wet/Dry mix
    │
    ├──> Click-free bypass
    │
    └──> Host state recall (must serialize ALL of the above)

MIDI-trigger mode (alternate sync model)
    │
    ├──> VST3 event input handling
    ├──> Retrigger / phase-reset logic
    │       │
    │       └──> Also used by audio-trigger mode
    └──> Mode switch UI (tempo-sync vs MIDI-trigger vs audio-trigger)

Audio-trigger mode (alternate sync model)
    │
    ├──> Transient detection DSP
    ├──> Retrigger / phase-reset logic
    └──> External sidechain audio input
            │
            └──> VST3 second audio bus

Multiband processing
    │
    ├──> Crossover filters (Linkwitz-Riley 4th order)
    ├──> Per-band curve playback engine (multiplies state)
    ├──> Per-band UI
    └──> (optional) Linear-phase filtering ──> latency reporting

Look-ahead
    │
    ├──> Dry signal delay buffer
    ├──> Latency reporting to host
    └──> Only meaningful with audio-trigger mode

Swing / groove offset ──> requires curve playback engine
Tempo offset / push-pull ──> requires curve playback engine
Oversampling ──> wraps the entire output gain stage
A/B compare ──> requires full state-snapshot mechanism

CLAP support ──> wraps DSP/UI; depends on framework choice (JUCE 7+, iPlug2 both have it)
```

### Dependency Notes

- **Curve playback engine is the foundation.** Tempo sync, MIDI trigger, and audio trigger are all just different ways of advancing the curve's position. Designing the engine to take a `position ∈ [0, 1]` source from a swappable clock module makes all three modes natural.
- **MIDI-trigger requires a different sync model than tempo-sync.** Pure tempo-sync derives playhead from `PPQ % curveLength`. MIDI-trigger resets playhead to 0 on note-on and either loops or one-shots. The two modes share evaluation but not advancement — design for both even if v1 only ships tempo-sync.
- **Audio-trigger needs transient detection.** Naïve threshold-based onset works for clean kicks; real-world drums need an envelope follower with attack/release smoothing or a proper onset detector. Cheap to start, easy to do badly.
- **Multiband requires crossover filters AND multiplies all per-band state** (curves, depth, mix). Doubles or triples the complexity of every feature it touches. PROJECT.md's decision to exclude it from v1 is well-aligned with the surveyed evidence — this is the single biggest scope multiplier in the category.
- **External sidechain depends on VST3 second audio bus.** Bitwig and most modern hosts support it; older hosts may not. Wired through, the DSP is identical to audio-trigger from main input.
- **Free-form curve editor enables nearly all "differentiator" curve features.** Randomize, multiple slots, import/export, undo/redo all assume an underlying free-form curve representation. Lock down the curve data model early.
- **Host state recall must serialize curves, not just parameter IDs.** Curves are not VST3 parameters in the conventional sense — they're structured data. The state save/load (`getState`/`setState`) must round-trip the full curve representation versioned for forward compatibility.
- **Look-ahead conflicts with low-latency tempo-sync.** Don't enable look-ahead when not in audio-trigger mode — it adds latency for no benefit.
- **A/B compare conflicts with parameter-only state.** A/B requires snapshotting the *entire* plugin state including curves. Build the snapshot mechanism for state recall, then A/B reuses it for free.
- **Click-free curve switching is non-obvious but critical.** When the user picks a different preset mid-playback, the gain envelope can jump discontinuously. Must crossfade or smooth at the moment of switch, not just on parameter changes.

---

## MVP Definition

This is the synthesis for the v1 milestone "bruikbaar voor productie in Bitwig on Apple Silicon". I'm matching to PROJECT.md's existing scope and flagging any places where the research suggests adjustment.

### Launch With (v1)

Already in PROJECT.md → Active. Research confirms these are the right minimum.

- [x] **Sample-accurate tempo-synced volume ducking** — foundation, no product without it
- [x] **Sync divisions: 1/1, 1/2, 1/4, 1/8, 1/16, dotted, triplet** — PROJECT.md specifies these. Note: this is *more* than Kickstart 2 itself ships (Kickstart only has 1/8, 1/4, 1/2, 1/1). 1/16 + dotted + triplet at near-zero implementation cost is a clear free win over Kickstart 2.
- [x] **Depth/scale knob (0-100%)** — table stakes
- [x] **Wet/dry mix** — table stakes; recommend keeping separate from depth (Kickstart conflates them, cleaner products separate)
- [x] **Click-free bypass, host-automatable** — table stakes; VST3 `kIsBypass` parameter with internal ramp
- [x] **All parameters automatable + state recall** — table stakes
- [x] **Built-in factory curves (Kickstart 1 family: sine, low pulse, high punch, sidechain-style)** — table stakes
- [x] **At least one user-editable curve slot** — minimum differentiator that separates from "Kickstart 1 clone with no soul"
- [x] **Visual curve display with playhead** — table stakes
- [x] **One-screen UI: curve, depth, sync, mix, bypass** — matches the "click-and-go" core value
- [x] **Click-free preset/curve switching** — table stakes (PROJECT.md implies via "no audible glitches/clicks under normal use")
- [x] **Undo of curve edits (in-plugin OR host-mediated)** — table stakes; PROJECT.md correctly leaves the choice open
- [x] **Bitwig project save/load with all state intact** — table stakes
- [x] **Native arm64 VST3** — platform constraint

**Recommended additions to v1 active list (research-driven):**

- [ ] **Resizable UI (at minimum 2-3 fixed sizes, e.g. 100% / 125% / 150%)** — table stakes in 2026. Cost is small if framework (JUCE/iPlug2) is used and UI is built in resolution-independent units from day one. Adding it later requires UI rework.
- [ ] **Simple A/B compare** — Cheap (S complexity) once state-snapshot mechanism for save/load is in place. Big perceived-quality win for "bruikbaar voor productie".

### Add After Validation (v1.x)

These are clearly within Kickstart 2's feature set but PROJECT.md correctly defers them. Adding incrementally once core is shipping cleanly.

- [ ] **MIDI-trigger mode** — currently in PROJECT.md Out of Scope but flagged "unless trivial to add". Research says: not trivial — requires alternate sync model. Defer to v1.1, but design the curve playback engine to accept different position sources so adding it later isn't a rewrite.
- [ ] **Multiple curve slots per instance (4-8 slots)** — modest extension once free-form curve editor is in place. LFOTool ships 12; that's overkill. 4-8 covers the realistic use case.
- [ ] **Curve import/export to local files** — cheap if curves are JSON. Lets users back up favorite shapes.
- [ ] **Randomize curve button** — small, fun, low-risk. Once the curve data model is settled, this is a few hours of work.
- [ ] **Tempo offset / push-pull timing (±50ms)** — small DSP add (just a position offset on curve evaluation), big musical value.
- [ ] **CLAP build alongside VST3** — Bitwig-native; framework-dependent cost. If JUCE 7+ or iPlug2, this is essentially a rebuild target. Worth doing in v1.x once VST3 is stable.

### Future Consideration (v2+)

These match PROJECT.md's Out of Scope and the research confirms they're the right things to defer.

- [ ] **Audio-trigger mode (transient-following)** — defining feature of Kickstart 2 marketing, but a substantial DSP project. Defer to v2 along with...
- [ ] **External sidechain audio input** — paired with audio-trigger; same DSP, different routing.
- [ ] **Multiband ducking (3 bands with crossover)** — biggest scope multiplier in the category. PROJECT.md correctly excludes from v1. v2 territory.
- [ ] **Look-ahead** — only meaningful with audio-trigger mode; ships together.
- [ ] **Swing / groove offset on curve** — moderate musical value, moderate complexity. v2.
- [ ] **Mid/side processing** — niche differentiator. v2 or "later own twist" territory.
- [ ] **Oversampling** — modulation aliasing is real but subtle on volume modulation. v2 or once a user complains.
- [ ] **Windows x64 build** — second milestone per PROJECT.md.
- [ ] **Codesigning, notarization, installer** — only relevant if commercial release becomes concrete. Per PROJECT.md.

---

## Feature Prioritization Matrix

| Feature | User Value | Implementation Cost | Priority |
|---------|------------|---------------------|----------|
| Tempo sync + sync divisions | HIGH | LOW | **P1** |
| Depth knob | HIGH | LOW | **P1** |
| Wet/dry mix | MEDIUM | LOW | **P1** |
| Click-free bypass | HIGH | LOW-MEDIUM | **P1** |
| Click-free curve/preset switching | HIGH | MEDIUM | **P1** |
| Parameter automation | HIGH | LOW | **P1** |
| Host state recall (with curve serialization) | HIGH | MEDIUM | **P1** |
| Built-in factory curves | HIGH | LOW | **P1** |
| One user-editable curve slot | HIGH | MEDIUM-HIGH | **P1** |
| Visual curve display + playhead | HIGH | MEDIUM | **P1** |
| Undo/redo (host-mediated acceptable) | MEDIUM | LOW-MEDIUM | **P1** |
| Native arm64 VST3 + Bitwig recall | HIGH | MEDIUM | **P1** |
| Resizable UI (fixed sizes) | MEDIUM | LOW (if planned from day 1) | **P1** |
| A/B compare | MEDIUM | LOW | **P2** |
| Multiple curve slots (4-8) | MEDIUM | LOW-MEDIUM | **P2** |
| Curve import/export (local files) | MEDIUM | LOW | **P2** |
| Randomize curve | LOW-MEDIUM | LOW | **P2** |
| Tempo offset / push-pull | MEDIUM | LOW | **P2** |
| CLAP build | MEDIUM (Bitwig native) | LOW-MEDIUM | **P2** |
| MIDI-trigger mode | HIGH | MEDIUM | **P2** |
| Audio-trigger mode | HIGH | HIGH | **P3** |
| External sidechain input | MEDIUM | MEDIUM | **P3** |
| Multiband (3 bands) | HIGH | HIGH | **P3** |
| Look-ahead (audio-trigger only) | MEDIUM | MEDIUM | **P3** |
| Swing / groove offset | MEDIUM | MEDIUM | **P3** |
| Mid/side processing | LOW-MEDIUM | MEDIUM | **P3** |
| Oversampling | LOW | MEDIUM | **P3** |
| Cloud presets / AI / standalone | LOW | HIGH | **Anti** |
| Full compressor / EQ / saturation | LOW | HIGH | **Anti** |
| MIDI generation / mod matrix | LOW | HIGH | **Anti** |

**Priority key:**
- **P1:** Must have for v1 launch (matches PROJECT.md Active scope + research-recommended additions)
- **P2:** Should have for v1.x — adds clear value for low-medium incremental cost
- **P3:** v2+ — significant scope additions, defer until v1 is validated
- **Anti:** Explicitly do not build

---

## Competitor Feature Analysis

| Feature | Kickstart 2 | VolumeShaper 6 | Duck 2 | LFOTool | Our v1 Approach |
|---------|-------------|----------------|--------|---------|-----------------|
| Tempo sync | Yes (1/8, 1/4, 1/2, 1/1 only) | Yes (broad divisions, multi-bar) | Yes (Lock to Beat) | Yes (BPM, swing, Hz) | Yes — wider divisions than Kickstart (incl. 1/16, dotted, triplet) |
| Curve editor | Limited — 16 fixed curves + draggable slope | Free-form, 40 nodes, snap-to-grid | Multi-segment with per-point shapes, brush, trace from audio | Free-form point + tension | Free-form, simple node + tension model (Xfer-style is cleanest) |
| Multiple curve slots | Per preset only | Per LFO (3 in multiband) | Envelope presets | 12 graphs/preset, MIDI-switchable | One slot v1; 4-8 in v1.x |
| Built-in presets | 16 hand-crafted (the headline) | Yes | Yes | Yes | Kickstart 1 family + a few more (PROJECT.md spec) |
| MIDI trigger | Yes | Yes | Yes | Yes (also CC out) | Defer to v1.x |
| Audio trigger | Yes (auto-follow non-4x4) | Yes (transient) | Yes (sidechain + audio + repeat) | No | Defer to v2 |
| External sidechain | Implicit via audio trigger | Yes (with kick overlay viz) | Yes | No | Defer to v2 |
| Multiband | Yes (band-split mode) | Yes (3 bands) | Yes (3 bands, linear-phase option) | No | Defer to v2 |
| Depth/Mix | "Mix" knob (conflated) | Separate per LFO | Separate | Separate | Separate depth + wet/dry (cleaner than Kickstart) |
| Click-free bypass | Yes | Yes | Yes | Yes | Yes (table stakes) |
| Resizable UI | Yes (75-200%) | Yes | Yes (free resize) | Limited | Yes (fixed sizes 100/125/150% is sufficient) |
| Undo/redo | Limited | Yes | Yes | Yes | Host-mediated for v1 |
| Randomize | No | Limited | Yes (multi-mode) | No | Defer to v1.x |
| Swing/groove | No | Limited | Yes (dynamic) | Yes | Defer to v2 |
| Look-ahead | No | No | Yes (unified offset) | No | Only with audio-trigger (v2) |
| Oversampling | Not advertised | Not advertised | Not advertised | Not advertised | Defer to v2 |
| A/B compare | No | No | No | No | Yes — easy win, no competitor has it |
| Pricing tier | $16 | $34 | $40 | $49 | TBD — clone aimed at Kickstart's tier if monetized |
| Plugin formats | VST2, VST3, AU, AAX | VST, VST3, AU, AAX | VST3, AU, AAX, CLAP | VST, AU, AAX | VST3 v1, +CLAP v1.x, Windows v2 |

**Strategic observation:** A/B compare is conspicuously absent from all four reference products. Cheap to build, immediate "feels professional" win. Worth keeping in v1 if cost is genuinely low after state-snapshot mechanism is built.

**Strategic observation 2:** Kickstart 2's deliberate limitation to four sync divisions (1/8, 1/4, 1/2, 1/1) is arguably its biggest practical weakness for modern genres. Adding 1/16, dotted, and triplet at essentially zero DSP cost is the cheapest way for a clone to feel *better than the reference* while staying in the same UX paradigm.

---

## Confidence Notes

- **Kickstart 2 feature set:** MEDIUM-HIGH. Confirmed via official site (kickstart-plugin.com — note site still markets as Kickstart 2 in May 2026, despite the project brief calling it "Kickstart 3"), MusicTech review, MusicRadar review, and consistent third-party reporting. Sync divisions specifically cross-verified.
- **VolumeShaper 6 feature set:** HIGH. Confirmed via official cableguys.com page and pluginboutique.
- **Duck 2 feature set:** HIGH. Confirmed via official deviousmachines.com Duck 2 page and Sound on Sound coverage.
- **LFOTool feature set:** MEDIUM. Confirmed via official xferrecords.com page and KVR community comparisons. Less recent than the others (LFOTool hasn't had a major update).
- **Pricing data:** MEDIUM. Pricing varies with sales/bundles; figures are May 2026 retail.
- **"Kickstart 3" vs "Kickstart 2" naming:** HIGH that the current product is Kickstart 2; the brief's "Kickstart 3" may reflect a misremembered version number or anticipation. Treat the brief's intent ("current Kickstart product as feature ceiling") as the binding spec.

---

## Sources

- [Kickstart official site (currently Kickstart 2)](https://www.kickstart-plugin.com/)
- [Cableguys VolumeShaper 6](https://www.cableguys.com/volumeshaper)
- [Cableguys ShaperBox 3](https://www.cableguys.com/shaperbox)
- [Devious Machines Duck 2](https://deviousmachines.com/product/duck2/)
- [Devious Machines — What's new in Duck 2](https://deviousmachines.com/2026/02/23/whats-new-in-duck-2/)
- [Xfer Records LFOTool](https://xferrecords.com/products/lfo-tool)
- [MusicTech — Cableguys Kickstart 2 review](https://musictech.com/reviews/plug-ins/cableguys-kickstart-2-review/)
- [MusicRadar — Nicky Romero/Cableguys Kickstart 2 review](https://www.musicradar.com/reviews/nicky-romerocableguys-kickstart-2)
- [Sound on Sound — Duck 2 from Devious Machines](https://www.soundonsound.com/news/duck-2-devious-machines)
- [Splice — Top 3 plugins for sidechaining](https://splice.com/blog/top-3-plugins-sidechaining/)
- [DJ TechTools — The Perfect Pump: 3 Sidechain Compression Plug-ins Compared](https://djtechtools.com/2015/05/24/the-perfect-pump-3-sidechain-compression-plug-ins-compared/)
- [KVR — LFO Tool vs VolumeShaper 6 / ShaperBox community comparison](https://www.kvraudio.com/forum/viewtopic.php?t=555819)

---

*Feature research for: Tempo-synced volume-ducking VST3 plugin (Kickstart-class)*
*Researched: 2026-05-03*
