# Module spec: build (BUILD)

Status: **approved for VS0-01 (reset, 2026-10-28)**; BUILD-01 reworded and BUILD-06 added in the reset.

## Acceptance criteria

- **BUILD-01** A fresh clone builds the VST3 and CLAP targets with `cmake -B build -G Ninja && cmake --build build` on macOS arm64; one `CMakeLists.txt`, dependencies via `FetchContent` pinned to a tag or SHA.
- **BUILD-02** Plugin loads in Bitwig (latest stable) on Apple Silicon as native arm64 (verified via `lipo -archs`)
- **BUILD-03** Plugin builds and ships as **CLAP** alongside VST3 via `clap-juce-extensions` (Bitwig is CLAP-native)
- **BUILD-04** GitHub Actions CI builds VST3 + CLAP on `macos-14` (arm64 runner) on every push
- **BUILD-05** Plugin name and identifiers are **Dukko** across CMake target, plugin manifest, and binary names
- **BUILD-06** Until the first DSP task lands, the plugin passes audio through unchanged (stereo and mono, in = out).
