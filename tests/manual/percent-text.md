# Bitwig: Depth and Mix read and take whole percentages (VS1-05, DSP-09)

Owner only. Copy to `tests/manual/runs/<date>-VS1-05.md`, tick, add `## Open points`.

- [ ] [Claude] Build installs the new Dukko; Bitwig rescans and loads it (CLAP) on an audio track.
- [ ] [Jij] Turn Depth: the device panel and the generic editor show whole values with one unit,
      e.g. `30 %`, never `30.0000000 %` or `30 % %`.
- [ ] [Jij] Type `30` into Depth (double-click the value): it reads `30 %`. Type `45 %` into Mix: it reads `45 %`.
- [ ] [Jij] Type `abc` into Mix: it jumps to `100 %`.
- [ ] [Claude] Same three checks in the VST3 (move `Dukko.clap` aside with Bitwig closed, see `bitwig-load.md`).
