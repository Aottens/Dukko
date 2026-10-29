# Bitwig: state survives save → close → reopen (VS1-03, STAT-04)

Copy to `tests/manual/runs/<date>-VS1-03.md`, tick, add `## Open points`.
[Claude] steps are setup Claude does; [Jij] steps need the owner in Bitwig.

- [ ] [Claude] Build the PR branch and install `Dukko.vst3` and `Dukko.clap` (`COPY_PLUGIN_AFTER_BUILD`).
- [ ] [Jij] Open a project saved before VS1-03 that holds Dukko (e.g. the VS0-01 or VS1-05 test
      project): it opens, Dukko shows the depth, mix and bypass it was saved with.
- [ ] [Jij] New project, Dukko (CLAP) on an audio track: set Depth 30 %, Mix 70 %, Bypass on;
      save, close the project, reopen it: Depth 30 %, Mix 70 %, Bypass on.
- [ ] [Jij] Same with the VST3 (Bitwig prefers CLAP; to test the VST3, move `Dukko.clap` aside
      with Bitwig closed, then put it back): Depth 45 %, Mix 20 %, Bypass off survive save → close → reopen.
- [ ] [Jij] In the old project from step 2, set Depth to 60 %, save, close and reopen: Depth 60 %
      (an old state migrates, saves as version 1 and reloads).
