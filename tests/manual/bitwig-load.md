# Bitwig: Dukko loads and passes audio (VS0-01, BUILD-02, BUILD-06)

Owner only. Copy to `tests/manual/runs/<date>-VS0-01.md`, tick, add `## Open points`.

- [ ] `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build` on a fresh clone succeeds.
- [ ] Bitwig rescans and shows Dukko (Dukko Audio) as VST3 and as CLAP.
- [ ] On an audio track, both formats load; the generic editor opens.
- [ ] Audio passes unchanged (A/B by toggling the device on and off: no level change).
- [ ] An existing project that already used Dukko still finds it (ids unchanged).
