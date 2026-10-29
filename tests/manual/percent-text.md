# Bitwig: Depth and Mix read as whole percentages (VS1-05, DSP-09)

Owner only. Copy to `tests/manual/runs/<date>-VS1-05.md`, tick, add `## Open points`.
Parsing typed text is covered by `dsp_09_depth_and_mix_read_and_parse_whole_percent`; Bitwig
offers no value entry in the device panel or the generic editor, so it is not part of this run.

- [ ] [Claude] Build installs the new Dukko; Bitwig rescans.
- [ ] [Jij] Load Dukko as CLAP and as VST3 on an audio track and turn Depth and Mix: the device
      panel and the generic editor show whole values with one unit, e.g. `30 %`, never
      `30.0000000 %` or `30 % %`.
