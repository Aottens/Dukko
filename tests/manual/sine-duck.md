# Bitwig: the sine duck pumps in time and follows Division (VS2-02, DSP-01)

Owner only. Copy to `tests/manual/runs/<date>-VS2-02.md`, tick, add `## Open points`.
The gain at known positions, the play head mapping and the 10 ms start/stop ramp are covered by
the `dsp_01_*` tests. Five minutes without drift, loops, jumps, tempo automation and clicks by ear
are VS2-04, not this run.

- [ ] [Claude] Build installs the new Dukko; Bitwig rescans.
- [ ] [Jij] Put a sustained sound (pad, noise or a long synth note) on a track at 120 bpm with
      Dukko (CLAP) after it, Depth and Mix 100 %, Division 1/4, and press Play: the volume
      pumps once per beat, quietest on each beat and back to full halfway between.
- [ ] [Jij] Switch Division to 1/8, then 1/2 while it plays: the pump goes twice as fast, then
      once per half note, still quietest on the beat.
- [ ] [Jij] Set Depth to 50 %: the pump stays in time but only goes halfway down.
- [ ] [Jij] Stop the transport while the sound keeps playing (e.g. hold a note): the duck
      stops and the sound is at full volume.
- [ ] [Jij] Repeat the first step with the VST3 version: same pump.
