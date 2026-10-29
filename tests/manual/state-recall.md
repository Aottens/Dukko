# Bitwig: an old project survives the new state loader (VS1-03, STAT-04)

Copy to `tests/manual/runs/<date>-VS1-03.md`, tick, add `## Open points`.
[Claude] steps are setup Claude does; [Jij] steps need the owner in Bitwig.

Save → close → reopen of Depth and Mix in CLAP and VST3 passed in the VS0-01 run (2026-10-28);
the round trip of every parameter and the migration are covered by the STAT-02/05/06 tests.
What only Bitwig can show is that a project saved before VS1-03 loads through the new code.

- [ ] [Claude] Build the PR branch and install `Dukko.vst3` and `Dukko.clap` (`COPY_PLUGIN_AFTER_BUILD`).
- [ ] [Jij] Open a project saved before VS1-03 that holds Dukko (e.g. the VS0-01 or VS1-05 test
      project): Dukko shows the Depth and Mix it was saved with.
- [ ] [Jij] In that project set Depth 60 % and Bypass on, save, close and reopen: Depth 60 %, Bypass on.
