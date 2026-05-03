# Architecture Research

**Domain:** Tempo-synced volume-ducking VST3/CLAP audio effect (Kickstart-class plugin)
**Researched:** 2026-05-03
**Confidence:** HIGH (well-established patterns in the JUCE / iPlug2 / nih-plug ecosystem; no exotic territory)

> Companion to `STACK.md` (chooses framework) and `FEATURES.md` (defines what to build). This file answers *how the code is organized and how data flows between threads at runtime*. Code snippets below are framework-agnostic pseudocode; they translate cleanly to JUCE `AudioProcessor`/`AudioProcessorValueTreeState`, iPlug2 `IPlug`/`IGraphics`, or nih-plug's `Plugin` trait.

---

## 1. Component Boundaries

The plugin is a single process loaded by the host, with two hard real-time boundaries: **(a)** the audio callback (running on a high-priority real-time thread owned by the host, must never allocate, lock, or block) and **(b)** the UI/message thread (event-driven, can do anything, runs at ~60 Hz redraw). Every component below sits cleanly on one side of that line, with a small set of explicitly-designed bridges between them.

| Component | One-line responsibility | Thread |
|-----------|------------------------|--------|
| **AudioProcessor (DSP core)** | Runs the per-block ducking: read transport → advance phase → evaluate curve → apply gain → wet/dry mix → output. | Audio |
| **ParameterManager** | Owns the host-visible parameter set (depth, mix, sync division, bypass, mode, curve-slot index). Backed by a value-tree state (JUCE `AudioProcessorValueTreeState` or equivalent). The single source of truth for automatable scalar values. | Both — atomic readers from audio, writers from UI/host |
| **ParameterSmoother** | Per-parameter one-pole / linear ramp running on the audio thread. Turns abrupt parameter changes into per-sample (or per-block) smoothed values. | Audio |
| **CurveModel** | Pure data: ordered list of `(x, y, tension)` nodes describing one curve, plus metadata (slot id, name, factory/user). No DSP logic, no UI logic. Plain copyable struct. | Both |
| **CurveEngine (evaluator)** | Stateless function `evaluate(curve, phase ∈ [0,1]) → gain ∈ [0,1]`. On audio thread reads from a *current* immutable `CurveSnapshot`; the UI thread publishes new snapshots via lock-free pointer swap. | Audio (reader), UI (writer) |
| **TransportSync** | Per-block bridge: reads host `ProcessContext` (BPM, PPQ, time-sig, transport state, sample rate, loop points) and produces, for the current block, a `phase_start`, `phase_increment_per_sample`, and `is_playing` flag for the curve evaluator. | Audio |
| **PresetManager** | Loads factory curve data (compiled into binary), manages user curve slots, handles import/export of curves to JSON files (v1.x). | UI / message |
| **StateSerializer** | Converts the *full* plugin state (parameter values + all curve slots + mode) to/from a versioned blob for `getState`/`setState` host calls. Single round-trip authority for project save/load. | UI / message (called by host) |
| **Editor (GUI root)** | The plugin's window. Hosts the curve-editor component, knobs, dropdowns, bypass, A/B button. Lifetime independent of AudioProcessor — host may open/close it freely while audio keeps running. | UI |
| **CurveEditorComponent** | Custom widget: draws the curve, hit-tests nodes, handles drag/add/delete, renders the live playhead. Reads `CurveModel` it owns; writes back to `CurveModel` and republishes a `CurveSnapshot` on every edit. | UI |
| **PlayheadProbe** | Audio thread writes the current normalized phase (single `std::atomic<float>`) every block; UI reads it on each repaint to draw the moving playhead line. | Audio (writer), UI (reader) |
| **VST3Wrapper / CLAPWrapper** | Format-specific glue (provided by JUCE/iPlug2/nih-plug). Translates host calls into our internal API. Should contain *zero* product logic. | Both (host-driven) |

### What talks to what — high-level component diagram

```
                             ┌──────────────────────────────────────────────────────────────┐
                             │                          HOST (Bitwig)                        │
                             │   transport • parameters • automation • state • UI window     │
                             └─────────────┬─────────────────────┬────────────────┬─────────┘
                                           │                     │                │
                            VST3/CLAP API  │                     │ getState/      │ open/close
                                           │ process()           │ setState       │ editor
                                           ▼                     ▼                ▼
              ┌───────────────────────────────────────┐  ┌─────────────────┐  ┌────────────────────────────┐
              │           AudioProcessor              │  │ StateSerializer │  │           Editor (GUI)     │
              │                                       │  │  (round-trip    │  │                            │
              │  ┌──────────────────────────────┐    │  │   versioned)    │  │ ┌────────────────────────┐ │
              │  │ TransportSync                 │    │  └────────┬────────┘  │ │ CurveEditorComponent   │ │
              │  │  reads ProcessContext         │    │           │           │ │  draws curve, handles  │ │
              │  │  → phase_start, phase_inc/spl │    │           │           │ │  drag, draws playhead  │ │
              │  └──────────────┬────────────────┘    │           │           │ └──────────┬─────────────┘ │
              │                 │                      │           │           │            │               │
              │  ┌──────────────▼────────────────┐    │           │           │ ┌──────────▼─────────────┐ │
              │  │ CurveEngine (evaluator)        │    │           │           │ │ Knobs / Sync dropdown  │ │
              │  │  reads CurveSnapshot* (atomic) │◀───┼───────────┼───────────┼─│ Bypass • A/B • Preset  │ │
              │  └──────────────┬────────────────┘    │           │           │ └──────────┬─────────────┘ │
              │                 │ gain[0..1]           │           │           │            │               │
              │  ┌──────────────▼────────────────┐    │           │           └────────────┼───────────────┘
              │  │ Gain stage + Wet/Dry + Bypass  │    │           │                        │
              │  │  (smoothed depth, mix)         │    │           │                        │
              │  └──────────────┬────────────────┘    │           │                        │
              │                 │ output samples       │           │                        │
              │                 ▼                      │           │                        │
              │           [host output buses]          │           │                        │
              │                                       │           │                        │
              │  ┌────────────────────────────┐       │           │                        │
              │  │ ParameterSmoother          │       │           │                        │
              │  └────────────┬───────────────┘       │           │                        │
              └───────────────┼───────────────────────┘           │                        │
                              │ atomic reads                       │                        │
                              ▼                                    ▼                        ▼
                    ┌──────────────────────────────────────────────────────────────────────┐
                    │                       ParameterManager (ValueTree)                    │
                    │   depth • mix • sync_div • mode • bypass • curve_slot • A/B index    │
                    │   single source of truth — atomics for audio, listeners for UI       │
                    └──────────────────────────────────────────────────────────────────────┘
                              ▲                                            ▲
                              │                                            │
                    ┌─────────┴────────────┐                  ┌────────────┴─────────────┐
                    │  CurveModel (data)   │◀────publish ─────│   PresetManager           │
                    │  per-slot node lists │   snapshot       │  factory + user curves    │
                    └──────────────────────┘                  └───────────────────────────┘
                              ▲
                              │ edit
                              │
                    ┌─────────┴────────────┐
                    │  CurveEditorComponent│
                    └──────────────────────┘

      Audio thread (real-time)            ←  hard boundary  →            UI / message thread
```

**Reading the diagram:**
- Solid downward arrows from `AudioProcessor` are the per-sample/per-block hot path. Nothing on that path may allocate or lock.
- `ParameterManager` is the single shared store. The audio thread reads via lock-free atomics that the framework's value-tree maintains; the UI thread reads/writes through normal listener APIs.
- The `CurveSnapshot` is published by the UI side (after curve edit or preset switch) and consumed by the audio side via a single atomic pointer swap. See §2 and §4.
- `StateSerializer` is the *only* component that talks to the host's state API; everything else exposes its state to it via plain getters.

---

## 2. Audio thread vs UI thread separation

### The rule

> **The audio thread never allocates, never takes a lock, never calls into the UI, and never blocks.** Everything else is a consequence of this.

Concretely, on the audio thread we forbid: `new`/`delete`/`malloc`, `std::mutex` (and any blocking primitive), `std::vector::push_back`, `std::string`, file I/O, logging that may allocate, calls into JUCE/iPlug components that touch the message manager, and (in nih-plug) anything that isn't `Send`-safe-and-realtime.

Allowed on the audio thread: stack allocation, fixed-size pre-allocated buffers owned by the processor, `std::atomic<T>` loads/stores with `relaxed` or `acquire`/`release` ordering, and pure math.

### Parameter flow UI → audio (the standard pattern)

```
[User turns Depth knob]
       │
       ▼
[UI: knob component] ── normalized value 0..1 ──▶ [ParameterManager.beginGesture / setValue / endGesture]
                                                              │
                                                              ▼
                                              ┌───────────────────────────────────┐
                                              │ ParameterManager (value tree)      │
                                              │  - holds std::atomic<float> per   │
                                              │    automatable parameter          │
                                              │  - notifies host (automation)     │
                                              └────────────────┬──────────────────┘
                                                               │ atomic store
                                                               ▼
[Audio thread, top of process()] ── atomic load ──▶ [ParameterSmoother.setTarget(newValue)]
                                                               │
                                                               ▼
                                              [Per-sample smoothed value used in DSP]
```

Three distinct mechanisms cooperate:

1. **Atomics for scalar parameters.** Every host-automatable parameter (depth, mix, bypass, sync division enum, mode enum, curve-slot index) is backed by an `std::atomic<float>` (or `<int>`). The audio thread reads it with `load(std::memory_order_relaxed)` once at the top of each process block (or once per smoother tick). No locking, no priority inversion.

2. **Per-sample smoothers for "knob-like" parameters.** Continuous parameters (depth, mix, bypass-ramp, tempo-offset) feed a one-pole or linear ramp smoother whose target is updated each block from the atomic, and whose output is read per sample in the inner loop. This eliminates zipper noise and bypass clicks. JUCE's `SmoothedValue<float, ValueSmoothingTypes::Linear>` is the canonical implementation; iPlug2 has `LogParamSmooth`; nih-plug has `Smoother`.
   - Bypass uses a longer ramp (5–10 ms) and a state-machine: `BypassRamping → Bypassed → BypassRamping → Active`. The DSP keeps running while ramping out, then short-circuits to dry once the ramp completes.

3. **Lock-free pointer swap for non-scalar state.** Curves, factory preset banks, and any structured state too big to fit in an atomic are published via a `std::atomic<std::shared_ptr<const CurveSnapshot>>` (C++20 `std::atomic<std::shared_ptr<T>>` is lock-free on Apple Silicon; if not, use a hand-rolled hazard-pointer or the `farbot::RealtimeObject` pattern from the JUCE community). The audio thread does `auto snap = current.load(std::memory_order_acquire);` once at the top of the block, holds a local `shared_ptr` for the duration of the block, and uses it for all curve evaluations within that block. The UI thread, after editing, builds a *new* immutable snapshot on the heap and `compare_exchange`s the pointer. The old snapshot is destroyed on whichever thread last drops it — safe because the audio thread only holds it transiently.

### Parameter flow audio → UI (much simpler)

The UI just polls. On a 60 Hz timer in the editor:
- Read playhead phase from a single `std::atomic<float>` written by the audio thread once per block.
- Read input/output peak levels from a pair of atomics.
- Repaint.

No queue is needed because the UI doesn't care about every sample, only the latest value.

### What is *not* needed for v1

- **MPSC queues for parameter changes.** Atomics suffice for scalar parameters. Queues are for messages, e.g. "user just clicked Randomize" — but Randomize fires on the UI thread, which can simply build a new `CurveSnapshot` and publish it. No queue.
- **Lock-free FIFOs for audio data.** v1 has no sidechain, no audio-rate communication between threads.
- **Reader-writer locks.** Anywhere we'd be tempted to use one, the snapshot pattern is simpler and faster.

A queue (e.g. `moodycamel::ReaderWriterQueue`) becomes worthwhile in v1.x when MIDI-trigger mode arrives: VST3 MIDI events are delivered to the audio thread and may want to be observed by the UI for visualization. That's a one-direction audio→UI fire-and-forget message, ideal for a single-producer single-consumer ring buffer.

---

## 3. Tempo-sync data flow

### What the host gives us each block

Every modern host (Bitwig, Ableton, Logic, Reaper, FL, Cubase) provides a `ProcessContext` (VST3) / `ITimeInfo` (iPlug2) / `Transport` (CLAP) struct on every audio process call. The fields we care about:

| Field | Type | Notes |
|-------|------|-------|
| `tempo` | double, BPM | May be automated by the host — can change *within* a block in extreme cases, but every host we target only updates it block-by-block. Treat as constant for the duration of one block. |
| `ppqPosition` | double | Position of the *first sample of this block* in quarter notes since song start. This is the heartbeat of tempo-sync. |
| `timeSigNumerator`, `timeSigDenominator` | int | Used only for display / sync-division semantics if we offer "per bar" division. |
| `isPlaying` | bool | If false, freeze the playhead. We still process audio (host may pass audio through stopped) but we don't advance phase. |
| `isLooping`, `loopStart`, `loopEnd` | bool, double, double | Looping creates discontinuities in `ppqPosition` — see "Loop and jump handling" below. |
| `sampleRate` | double | Provided at `prepareToPlay`; assume constant across blocks. |
| `numSamples` | int | Block size. |

### Computing sample-accurate phase

The curve has a *length in quarter notes*: `curve_qn_length = note_division_to_qn(sync_division)`, e.g. 1/4 → 1.0 qn, 1/8 → 0.5 qn, 1/16 → 0.25 qn, 1/4 dotted → 1.5 qn, 1/8 triplet → (0.5 × 2/3) ≈ 0.333 qn.

For each block:

```
// Inputs from host this block
ppq_block_start  : double   // ProcessContext.ppqPosition
bpm              : double   // ProcessContext.tempo
sample_rate      : double
num_samples      : int

// Inputs from parameter state (atomic loads, smoothed if needed)
sync_division    : enum
tempo_offset_qn  : double   // 0 in v1; ±0.05 in v1.x

// Derived
curve_qn_length            = division_to_qn(sync_division)
ppq_advance_per_sample     = bpm / 60.0 / sample_rate         // qn per sample
phase_at_block_start       = fmod((ppq_block_start + tempo_offset_qn) / curve_qn_length, 1.0)
if phase_at_block_start < 0: phase_at_block_start += 1.0      // fmod can be negative
phase_increment_per_sample = ppq_advance_per_sample / curve_qn_length

// Per-sample inner loop
phase = phase_at_block_start
for n in 0..num_samples:
    gain      = curve_eval(snapshot, phase)         // ∈ [0,1], 1 = no duck
    depth     = depth_smoother.next()
    mix       = mix_smoother.next()
    duck_gain = 1.0 - depth * (1.0 - gain)          // depth scales the duck
    wet       = input[n] * duck_gain
    output[n] = lerp(input[n], wet, mix)             // wet/dry
    phase    += phase_increment_per_sample
    if phase >= 1.0: phase -= 1.0
// Publish final phase for UI playhead
playhead_atomic.store(phase, std::memory_order_relaxed)
```

This is **sample-accurate** because we recompute phase from the host's authoritative `ppqPosition` at the start of every block, and within a block we advance by `phase_increment_per_sample` derived from the host's BPM. The only floating-point drift is within a single block (≤ a few hundred microseconds at typical buffer sizes), and that drift is *reset to ground truth* on the next block. Over the course of a song the playhead is locked to the host's musical clock to within sub-sample accuracy.

### Loop and jump handling

The `ppqPosition` field handles the hard cases for us:
- **Host loop bracket:** when the host wraps from `loopEnd` back to `loopStart`, the next block's `ppqPosition` simply *jumps backwards*. Because we recompute `phase_at_block_start = fmod(ppq / curve_qn_length, 1.0)` every block, the curve picks up at exactly the right phase for the new position. No glitch.
- **User clicks the timeline / jumps the playhead:** same mechanism. The block after the jump has a new `ppqPosition`; we relock.
- **Tempo automation:** `bpm` may be different in consecutive blocks. Because we re-derive `phase_increment_per_sample` from `bpm` each block, the playhead speeds up / slows down at block boundaries. This is *audibly indistinguishable* from per-sample tempo for tempo-curve modulation; the artifact would only matter for sub-block ramping which no host does.
- **Transport stopped:** when `isPlaying == false`, freeze: don't advance phase, hold the last gain value (don't reset to 1.0 — that would cause a step). On `isPlaying` going true → recompute phase from host PPQ; the ramped depth-smoother absorbs any discontinuity.
- **Free-running mode (no host playing or no PPQ):** v1 doesn't ship this, but design `TransportSync` to fall back to a free-running internal counter at host BPM (or a default 120 BPM if `bpm == 0`) when `isPlaying` is false. Keeps things working in unusual hosts and during tests.

### Why a "per-block snapshot" is the right granularity

We could try to honour intra-block tempo or PPQ changes, but no current host provides that information. The block-boundary update gives sample-accurate sync with sub-sample drift, which is well below any audible threshold for volume modulation. Don't over-engineer this — it's the universally correct pattern in JUCE/iPlug2/nih-plug ducking and LFO plugins.

---

## 4. Curve representation

### Decision: **point + tension (Xfer/LFOTool model)**

A curve is an ordered list of `(x, y, tension)` tuples where:
- `x ∈ [0, 1]` is normalized phase, strictly increasing, with `x[0] = 0` and `x[N-1] = 1` (closed-loop curve).
- `y ∈ [0, 1]` is the gain at that node.
- `tension ∈ [-1, 1]` controls the shape of the segment *to the next node*: `0` is linear, positive bows the segment one way (concave up), negative bows the other (convex). Eight different `tension`-driven segment shapes can be exposed in UI (linear, ease-in, ease-out, S-curve, etc.) but the underlying math is one parameter.

Per-node payload is 12 bytes (3 × float32) plus a 1-byte segment-shape enum. A 16-node curve fits in well under a cache line.

### Why not the alternatives

| Option | Verdict | Reason |
|--------|---------|--------|
| **Bezier with explicit control handles** (2 vec2 handles per node) | No | Doubles the storage, doubles the UI hit-targets, and users hate dragging four handles per point. VolumeShaper does this and it's the most-complained-about part of its UI. |
| **Per-segment shape enum (Duck-2-style)** | No (alone) | Less expressive than tension; users want to dial *between* "linear" and "ease-in" smoothly. We can offer the *enum* in the UI as named tension presets, but the underlying data must be the continuous tension value to round-trip cleanly. |
| **Precomputed LUT (e.g. 1024-sample float array)** | No (as primary representation) | Cheap evaluation but lossy on round-trip (re-edit → re-bake → re-edit accumulates error), opaque to serialization (huge JSON, no diff-friendliness), and prevents per-sample interpolation accuracy. *Use it as a derived cache* (see below) but not as the source of truth. |
| **Function evaluator (math expression DSL)** | No | Power-user feature; v1 doesn't need it; opens a security/stability rabbit hole (eval safety, edge cases, JIT). |

### The `CurveSnapshot` — what the audio thread actually sees

The CurveModel (point + tension list) is **not** what the audio thread evaluates directly. We bake it into a tiny precomputed structure once per edit (on the UI thread) and the audio thread reads only this immutable snapshot:

```cpp
struct CurveSnapshot {
    static constexpr int LUT_SIZE = 1024;       // 4 KB at float32
    std::array<float, LUT_SIZE> lut;            // gain(phase) sampled uniformly
    // Optional: keep node list for click-free crossfade between snapshots
    std::array<Node, MAX_NODES> nodes;
    int num_nodes;
    uint32_t version_id;                        // monotonic, for debug
};
```

Audio-thread evaluation is then a single LUT lookup with optional linear interpolation:

```cpp
float curve_eval(const CurveSnapshot& s, float phase) {
    float idx_f = phase * (CurveSnapshot::LUT_SIZE - 1);
    int   i0    = (int)idx_f;
    int   i1    = (i0 + 1) & (CurveSnapshot::LUT_SIZE - 1);  // wrap, LUT_SIZE is power of 2
    float frac  = idx_f - i0;
    return s.lut[i0] + frac * (s.lut[i1] - s.lut[i0]);
}
```

This buys us: O(1) audio-thread cost (one branchless LUT lookup), full control over UI evaluation (the UI evaluates the *exact* point+tension math when drawing, so users see the analytic curve, not the LUT), and trivial click-free preset switching (we crossfade two snapshot LUTs over ~5 ms during a swap).

**Source of truth:** the `CurveModel` (node list). **What's serialized:** the `CurveModel`. **What the audio thread sees:** the derived `CurveSnapshot`. The UI rebuilds the snapshot any time the model changes, which is rare (tens of times per second at most during a drag).

### Audio-thread cost

At 48 kHz, 256-sample buffer: 256 LUT lookups × 2 multiplications each = ~512 FLOPs per block for curve evaluation, plus a small constant for the gain stage. Comfortably under 0.1% CPU per instance on M-series.

---

## 5. State / preset architecture

### Two concerns, one mechanism

The plugin must persist:
1. **Parameter values** (depth, mix, sync division, mode, bypass, etc.) — already handled by the framework's value tree.
2. **Curve geometry** for every slot (v1: factory bank read-only + 1 user slot; v1.x: 4–8 user slots).

VST3 / CLAP provide `getState(stream)` / `setState(stream)` and the host calls them on project save/load. The framework wraps these as `getStateInformation`/`setStateInformation` (JUCE) or `serialize`/`deserialize` (nih-plug). Whatever the wrapper, the contract is: "give me a blob of bytes; restore from a blob of bytes."

### Format: **XML inside the framework's value tree**

Use the framework's native XML (JUCE) or equivalent valuetree-as-bytes (iPlug2 chunks, nih-plug's `serde_json`). Reasons:
- The parameter half is already XML/valuetree-shaped — adding a `<curves>` child to it is one line.
- XML is diff-friendly in version control, which matters for factory presets shipped in the binary.
- Human-readable when debugging "why doesn't Bitwig restore my preset" — open the project file (Bitwig stores plugin chunks base64-inside its `.bwproject`), decode, eyeball.
- Forward compatibility is straightforward via a top-level version attribute.

The serialized blob looks like:

```xml
<KickstartCloneState version="1">
  <Parameters>
    <PARAM id="depth"   value="0.75"/>
    <PARAM id="mix"     value="1.0"/>
    <PARAM id="sync"    value="3"/>      <!-- enum index for 1/8 -->
    <PARAM id="mode"    value="0"/>
    <PARAM id="bypass"  value="0"/>
    <PARAM id="slot"    value="0"/>
  </Parameters>
  <Curves>
    <Slot index="0" name="Custom 1">
      <Node x="0.0"  y="0.0"  tension="0.5"/>
      <Node x="0.15" y="0.95" tension="-0.3"/>
      <Node x="1.0"  y="1.0"  tension="0.0"/>
    </Slot>
    <!-- ... more slots in v1.x ... -->
  </Curves>
  <ABState>
    <!-- v1.x: full snapshot of the "other" A/B side, same shape as above -->
  </ABState>
</KickstartCloneState>
```

Versioning rule: increment `version` when the schema changes. `setState` checks the version and migrates older blobs forward. Factory curves are *not* serialized — they're compiled into the binary as static data; only the `slot` index references them. This keeps blob size tiny (a few hundred bytes for a typical project) and means factory updates ship transparently.

### Why not JSON

JSON is fine semantically but the framework's XML is already there for parameters; mixing two serialization formats in one blob doubles the surface area. If commercial v2 ever wants to expose *external* preset files (`.kickclone-preset`), use JSON for those — they're independent of the in-DAW state blob.

### Why not raw binary

Writes a few bytes faster and reads a few bytes smaller. Costs: opaque debugging, brittle versioning, painful in code review. Volume modulation plugins are not size-constrained; the trade-off is bad for us.

### Bitwig-specific notes

- Bitwig is **stricter than most hosts** about state recall. It calls `setState` very early in project load, before audio starts, and again on plugin re-instantiation. Our `setState` implementation must be fully deterministic and idempotent — calling it twice with the same blob must produce identical state.
- Bitwig **base64-encodes** the chunk inside its project file. We don't need to do anything special; our blob just has to round-trip cleanly through opaque bytes.
- Bitwig will rapidly toggle automation lanes during scrubbing; the parameter atomics already handle this, but make sure no `setState` path triggers UI invalidation that allocates on the host's calling thread (some Bitwig versions call into us from a non-message thread on certain operations).
- Bitwig respects the VST3 `kIsBypass` flag and provides project-level bypass. Our internal bypass parameter must be wired through this flag (JUCE's `AudioProcessorParameter::isMetaParameter` / iPlug2's `IsBypass`), not as a regular bool.

### A/B compare (v1.x)

Two `KickstartCloneState` blobs in memory: `state_a`, `state_b`. The "active" one is what drives the audio. Pressing A/B swaps a single index. Pressing "copy A→B" memcopies. This sits naturally on top of the existing serialization mechanism — it's the same data shape — so we get it almost free once `setState`/`getState` are solid. The A/B index itself is a parameter so it's automatable too.

---

## 6. GUI architecture

### One window, fixed regions

Per `FEATURES.md`: one-screen UI with curve display, depth, sync division, mix, bypass. Resizable in fixed steps (100% / 125% / 150%) for v1.

```
┌────────────────────────────────────────────────────────────────────┐
│  KickstartClone                              [A][B]   [⚙]  [BYPASS]│
├────────────────────────────────────────────────────────────────────┤
│  Preset: [▼ Sidechain Classic     ]    Slot: [User 1] [Save] [⏪]  │
├────────────────────────────────────────────────────────────────────┤
│                                                                    │
│   ┌────────────────────────────────────────────────────────┐      │
│   │                                                          │      │
│   │            ●─────────╮                                   │      │
│   │                       ╲                                  │      │
│   │                        ╲                                 │      │
│   │                         ●─────────────●─────────────●    │      │
│   │                              ▲                           │      │
│   │                              │ playhead                  │      │
│   │   CurveEditorComponent                                   │      │
│   └────────────────────────────────────────────────────────┘      │
│                                                                    │
├────────────────────────────────────────────────────────────────────┤
│   SYNC: [▼ 1/4 ]    DEPTH: ◉ 75%    MIX: ◉ 100%    [Randomize]    │
└────────────────────────────────────────────────────────────────────┘
```

### Component tree

```
Editor
├── HeaderBar
│   ├── PluginNameLabel
│   ├── ABCompareButton
│   ├── SettingsButton           // window size, future options
│   └── BypassButton              // bound to bypass parameter
├── PresetBar
│   ├── PresetDropdown            // factory presets, populates curve
│   ├── SlotSelector              // v1: User 1 only; v1.x: User 1..8
│   ├── SaveSlotButton
│   └── UndoButton                // host-mediated; sends parameter undo events
├── CurveEditorComponent          // the heart of the UI
│   ├── (paints) CurveModel.nodes
│   ├── (paints) playhead line, polled from PlayheadProbe atomic
│   ├── (handles) mouse drag → CurveModel mutation → publish CurveSnapshot
│   └── (handles) double-click add node, right-click delete, alt-drag tension
└── ControlsBar
    ├── SyncDropdown               // enum parameter: 1/1, 1/2, 1/4d, 1/4, 1/4t, 1/8d, 1/8, 1/8t, 1/16d, 1/16, 1/16t
    ├── DepthKnob                  // 0..100%
    ├── MixKnob                    // 0..100%
    └── RandomizeButton            // v1.x — generates new node positions/tensions, snaps to grid
```

### Where the playhead lives

The playhead indicator is **drawn by `CurveEditorComponent`** on top of the curve. It reads a single `std::atomic<float>` (`PlayheadProbe`) on a 60 Hz `Timer` callback (JUCE) / `RunLoop` (iPlug2) and triggers a localized repaint of just the column containing the previous and current playhead positions. Don't repaint the whole curve every frame — only the playhead overlay needs to update at 60 Hz; the curve geometry only changes on edit.

### Resizable UI without rework

Build the UI with **resolution-independent units**. Two patterns work:
- **JUCE:** Use `Component::setBounds` based on a logical unit (e.g. 1 unit = 8 px @ 100%) and apply an `AffineTransform::scale()` at the editor root. All children stay in logical coordinates.
- **iPlug2:** Use `IGraphics` with `SetScreenScale()` and design the UI in logical pixels.

Set scale once in the settings; the framework scales the entire hierarchy. Mouse coordinates come back pre-scaled.

### What the GUI **does not** do

- Audio processing (obvious, but worth stating: no DSP code in the editor).
- Direct mutation of `AudioProcessor` state (always goes through ParameterManager or CurveSnapshot publication).
- Heavy work on paint (no curve LUT recomputation on every paint — that's done once per edit).
- File I/O on the message thread for preset import/export — push to a background thread.

---

## 7. Build order

The build order minimizes rework risk by establishing the cross-thread plumbing *before* layering features on top.

### Phase A — Plumbing & DSP scaffold (week 1)

> Goal: an empty plugin that loads in Bitwig, has automatable parameters, runs trivial DSP, and survives project save/load.

1. **Project scaffold.** Pick framework (per `STACK.md` — JUCE / iPlug2 / nih-plug), generate a "passthrough effect" template, build native arm64, validate it loads in Bitwig.
2. **ParameterManager + StateSerializer.** Define the full parameter set up front (depth, mix, sync_division, mode, bypass, curve_slot). Wire to the framework's value-tree state. Implement `getState`/`setState` round-trip with a version field. Test save/load in Bitwig with a parameter changed.
3. **AudioProcessor with passthrough + bypass + wet/dry + depth.** No curve yet — just `output = lerp(input, input * (1 - depth), mix)`, with smoothers on depth/mix/bypass. Verify: click-free bypass, click-free depth changes, automation works in Bitwig.

> **Exit criterion:** plugin loads, parameters automate, project save/load works, no clicks. This is the *least-fun* part of the project, but every later component depends on it being correct.

### Phase B — Curve engine and tempo sync (week 2)

> Goal: a hardcoded sine-curve duck that's tempo-locked to the host.

4. **CurveModel + CurveSnapshot + bake function.** Implement node list → LUT bake on the UI thread. Hardcode one factory curve (sine-style) for now.
5. **TransportSync.** Read `ProcessContext`, compute `phase_start` and `phase_increment_per_sample`. Test with Bitwig at various BPMs and after timeline jumps.
6. **CurveEngine (audio-thread evaluator) + atomic snapshot publication.** Wire the LUT lookup into the gain stage. Verify: dropping the plugin on a track with a kick at 1/4 produces a tight pumping effect, locked to the grid, with no drift over many bars.
7. **PlayheadProbe atomic.** Audio thread writes phase each block.

> **Exit criterion:** plugin ducks audio at host tempo, sync_division parameter changes the rate, no audible drift after 5 minutes of playback, bypass still clean.

### Phase C — Curve UI and factory presets (week 3)

> Goal: visual curve, working playhead, factory preset dropdown.

8. **CurveEditorComponent — read-only paint.** Draws the current `CurveModel` and overlays the playhead from `PlayheadProbe`. No editing yet.
9. **Factory preset bank.** Bake the Kickstart-1 family (sine, low pulse, high punch, sidechain classic) as `static constexpr` node arrays in the binary. Wire the preset dropdown to swap which `CurveSnapshot` is active. Verify: click-free preset switching during playback (5–10 ms LUT crossfade between snapshots).
10. **Resizable UI scaling.** Add settings menu with 100/125/150% scale, validate on Retina display.

> **Exit criterion:** the plugin is *visually complete for a user who only uses factory presets*. This is "Kickstart 1 clone" territory — already useful.

### Phase D — Editable curve and curve serialization (week 4)

> Goal: the user can edit a custom curve and have it survive save/load.

11. **CurveEditorComponent — editing.** Drag nodes, alt-drag for tension, double-click to add, right-click to delete. On every mutation: rebuild snapshot, publish via atomic swap.
12. **One user curve slot.** Add `<Curves>` to the state blob, version 1. Verify: edit → save Bitwig project → reopen → curve restored exactly.
13. **In-plugin undo OR host-mediated undo.** Decide based on framework cost. JUCE's `UndoManager` integrates with `AudioProcessorValueTreeState` cleanly; iPlug2 needs a hand-rolled stack. For v1, host-mediated is acceptable per `PROJECT.md`.
14. **Click-free curve switching during edits.** Verify: dragging a node mid-playback causes no clicks (the snapshot crossfade handles this).

> **Exit criterion:** the v1 acceptance test in `PROJECT.md` is met. Plugin is "bruikbaar voor productie" in Bitwig.

### Phase E — v1 polish (week 5)

15. **A/B compare.** Two state blobs in memory, one swap parameter.
16. **Bypass and edge-case audit.** Verify behaviour during host stop, loop, jump, tempo automation, sample-rate change at runtime.
17. **CPU profile.** Confirm <1% per instance at 48 kHz / 256 samples on M-series.
18. **VST3 validator.** Run the official VST3 test suite (`validator` from the SDK). Fix anything it complains about.
19. **Bitwig stress test.** Multiple instances, automation lanes, project save/load cycle 10 times, scrubbing, tempo-curve automation.

### Why this order

- **Plumbing before DSP:** the cross-thread architecture is the hardest thing to retrofit. Getting it wrong means rewriting every later component. Establish it on a trivial passthrough.
- **DSP before UI:** without working ducking, there's nothing for the UI to display. A "test the audio with parameter automation only" milestone is *valuable* — many DSP bugs are easier to find without UI noise.
- **Read-only UI before editor:** the editor introduces UI→audio data flow on top of UI←audio (playhead). Solve one direction at a time.
- **Factory presets before editable curve:** factory curves exercise snapshot publication and click-free switching without the UX complexity of node editing. If snapshot swap is broken, you find it here, not while debugging "why does my edit jump the audio."
- **Custom curve last:** by the time you implement node editing, the snapshot mechanism is already battle-tested.

### Dependency graph

```
ParameterManager + StateSerializer
        │
        ├──▶ AudioProcessor (passthrough+depth+mix+bypass)
        │           │
        │           ├──▶ TransportSync ──▶ CurveEngine ──▶ Factory CurveSnapshot
        │           │                                              │
        │           │                                              ▼
        │           │                                  CurveEditorComponent (read-only)
        │           │                                              │
        │           │                                              ▼
        │           │                                  CurveEditorComponent (editable)
        │           │                                              │
        │           ▼                                              ▼
        │   PlayheadProbe ───────────────────────────▶  Live playhead overlay
        │
        └──▶ Curve serialization (extends state blob)
                    │
                    ▼
            A/B compare (reuses state blob)
```

---

## 8. Cross-platform readiness

v1 is Apple Silicon only, but `PROJECT.md` requires the codebase to reach Windows x64 in milestone 2 without a rewrite. Decisions made now that pay off later:

### Choices that pay off

- **Cross-platform framework from day one.** JUCE, iPlug2, and nih-plug all target macOS+Windows from the same source. Pick one (per `STACK.md`) and you get Windows for the cost of a CI runner and an installer.
- **No Apple-specific APIs in product code.** Don't reach for Accelerate framework (use the framework's portable DSP), don't use Core Foundation strings (use `juce::String` / `std::string` / Rust `String`), don't use AVFoundation, don't use Cocoa for UI. The framework provides portable equivalents.
- **CMake-based build (or Projucer/cargo with platform-agnostic config).** Avoid Xcode-specific build steps. CMake builds the same project on Apple Silicon and Windows MSVC with a single command swap.
- **Endianness-safe state serialization.** XML/text is endian-independent (good). If we used raw binary, we'd need explicit little-endian normalization. Stick with text.
- **No assumptions about file paths.** Use `juce::File::getSpecialLocation` / `iplug::FileLocation` / Rust `dirs` crate for user-data folders. Hard-coded `~/Library/...` paths break the moment the build runs on Windows.
- **Resolution-independent UI.** Windows DPI varies wildly (96 / 120 / 144 / 192 DPI). Designing the UI in logical units from day one (per §6) means it works on a 4K Windows display without rework.
- **Atomic types from `<atomic>`, not platform intrinsics.** `std::atomic<T>` is portable; `OSAtomicAdd32Barrier` is not.
- **Threading via `<thread>` / `std::thread` / framework wrappers.** `pthread_*` is fine on macOS but doesn't exist as such on MSVC.
- **VST3 SDK plus an optional CLAP wrapper from the framework.** Both target Windows; nothing macOS-specific.
- **Compile arm64 *and* x86_64 from the start (universal binary on macOS).** Even if v1 is "Apple Silicon only," configuring the build to emit a universal binary is one CMake flag and proves the code isn't accidentally arm64-specific. This rules out a class of "works on M1, fails on x86" surprises before they happen.

### Choices that would hurt

- **Apple-only audio APIs.** AU/AudioToolbox: explicitly out of scope already. If you reach for `AudioUnit*` types in product code (e.g. for DSP), Windows is dead.
- **macOS-specific UI toolkits.** Cocoa/SwiftUI/Metal-direct: all portless. The framework's UI is the right level of abstraction.
- **Filesystem assumptions.** `/Users/...`, `/Library/Audio/Plug-Ins/...`. Use the framework's "user data dir" abstraction.
- **Path separator hardcoding.** Use `std::filesystem::path` (C++17) which handles `/` vs `\`. Don't `+ "/"` strings together.
- **Code-signing scripts in build pipeline.** Don't make the macOS code-sign step a *required* part of `cmake --build`; gate it behind a CI flag. Otherwise Windows builds choke on `codesign` not being available.
- **Hardcoded plugin paths.** Bitwig on macOS: `~/Library/Audio/Plug-Ins/VST3/`. Bitwig on Windows: `C:\Program Files\Common Files\VST3\`. The framework's "install plugin" CMake target handles both — use it, don't roll your own.

### Pre-Windows-port checklist (run at end of v1)

Even though the port is milestone 2, run these checks at end of v1 to catch regressions early:

- [ ] Project builds on a clean Windows MSVC environment from CMake (CI).
- [ ] All `std::filesystem::path` usage compiles under MSVC's stricter rules.
- [ ] No literal `\n` line-ending assumptions (state-blob diffing).
- [ ] No `__attribute__((aligned))` without an MSVC `__declspec(align)` fallback.
- [ ] No GCC-specific intrinsics; use `std::atomic`, `std::bit_cast`, etc.
- [ ] Static analysis pass on Windows (MSVC has different warnings than Clang).
- [ ] Plugin loads in REAPER on Windows (free, fast smoke test before installing Bitwig).

---

## Anti-Patterns

### Anti-Pattern 1: "Just one little lock" on the audio thread

**What people do:** A shared `std::mutex` around the curve list, locked briefly in `processBlock` and held while the UI edits.
**Why it's wrong:** Priority inversion. The audio thread runs at the highest priority on the system; if it tries to acquire a lock held by a lower-priority UI thread, the OS will block the audio thread until the UI thread is rescheduled. On macOS that can be 10+ ms, far longer than a single audio buffer. Result: dropouts and CPU spikes that are nearly impossible to reproduce in tests.
**Do this instead:** Lock-free atomic snapshot pointer (§2 and §4). The audio thread holds an immutable view; the UI thread builds a new view and atomic-swaps. No locks ever cross the thread boundary.

### Anti-Pattern 2: Computing phase from a free-running counter

**What people do:** "I'll just count samples and compute `phase = sample_count * bpm / sample_rate / qn_per_curve`."
**Why it's wrong:** Diverges from host PPQ on every transport jump, loop, scrub, tempo automation. The plugin slowly drifts out of sync. Symptom: "the duck is offset from the kick on bar 17 of the project."
**Do this instead:** Recompute phase from `ProcessContext.ppqPosition` at the start of every block (§3). The host's PPQ is the authoritative musical clock.

### Anti-Pattern 3: Allocating in `processBlock`

**What people do:** `std::vector<float> tempBuffer(numSamples);` at the top of process, or `std::string log = ...;` for debugging.
**Why it's wrong:** `malloc` can take milliseconds under memory pressure. On the audio thread, that's an underrun.
**Do this instead:** Pre-allocate every buffer in `prepareToPlay`. For debugging, use a lock-free ring buffer that the UI drains. JUCE's `ScopedNoDenormals` and `AudioBuffer::clear` are allocation-free; use them.

### Anti-Pattern 4: Letting the UI thread directly mutate audio-thread state

**What people do:** Curve dragging directly writes into the live curve being evaluated.
**Why it's wrong:** Tearing — the audio thread reads a half-updated curve and outputs garbage for one block.
**Do this instead:** UI builds a complete new immutable snapshot, then atomic-swaps the pointer. Audio thread always sees a coherent snapshot.

### Anti-Pattern 5: Storing factory curves in the project blob

**What people do:** Serialize all 16 factory curves into every saved project.
**Why it's wrong:** Bloats the project file. When v1.1 ships an updated factory curve, every existing project still has the old one frozen in. Users can't get the new shapes without re-saving.
**Do this instead:** Serialize only an *index* into the factory bank. The curves live in the binary. Updating curves means shipping a new binary; projects pick them up automatically. (User-edited curves are serialized in full because they're authoritative.)

### Anti-Pattern 6: Tying the editor lifetime to the AudioProcessor

**What people do:** Putting state that the audio needs inside the editor class.
**Why it's wrong:** The host can close and reopen the editor at any time; state inside the editor is destroyed and recreated. If the audio depends on it, audio breaks every time the user closes the window.
**Do this instead:** All audio-relevant state lives in the AudioProcessor and ParameterManager. The editor is *only* a view — it observes and emits events. Closing the editor must have zero audible effect.

### Anti-Pattern 7: Bypass via `if (bypass) return;` at the top of process

**What people do:** Check the bypass parameter, return early, skip DSP.
**Why it's wrong:** (a) Doesn't ramp — clicks on toggle. (b) Doesn't preserve internal DSP state — when bypass releases, smoothers are stale and click again. (c) VST3 hosts expect bypassed plugins to still pass audio through with the *same latency* as active; early-returning may break compensation.
**Do this instead:** Always run the full DSP path. Use a smoothed bypass parameter that ramps from 1.0 (active) to 0.0 (bypassed) over 5–10 ms, and crossfade between the processed signal and the dry input using that ramp. Internal DSP state stays current; transitions are inaudible.

### Anti-Pattern 8: Trusting `bpm == 0` as "no host"

**What people do:** Assume the host always provides a valid BPM.
**Why it's wrong:** Some hosts (older Reaper, certain CLAP test hosts, AAX in offline render) report `bpm = 0` or `isPlaying = false` for prolonged periods. Dividing by it gives NaN; the audio thread starts producing NaN samples; the plugin appears to "destroy" audio.
**Do this instead:** Defensive check in `TransportSync`: if `bpm < 1.0` or `sample_rate <= 0`, freeze the playhead at its last position and pass dry. Never divide by an unverified host value.

---

## Sources

- Primary: established patterns from JUCE, iPlug2, and nih-plug ducking/LFO plugin codebases. These three frameworks dominate VST3 plugin development as of 2026 and converge on the same architectural choices: atomic snapshot publication, per-block transport reads, smoothed parameters, value-tree state.
- VST3 SDK documentation (Steinberg) — `ProcessContext`, `getState`/`setState`, `kIsBypass`.
- CLAP specification (free-audio.github.io/clap) — transport struct, parameter API.
- Bitwig DAW behaviour observations from `FEATURES.md` research.
- Confidence: HIGH on patterns (these are the de-facto standards), MEDIUM on Bitwig-specific edge cases (worth re-validating during phase A in actual Bitwig).

---

*Architecture research for: Tempo-synced volume-ducking VST3/CLAP plugin (Kickstart-class)*
*Researched: 2026-05-03*
