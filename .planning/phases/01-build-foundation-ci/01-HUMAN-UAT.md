---
status: passed
phase: 01-build-foundation-ci
source: [01-VERIFICATION.md]
started: 2026-05-03T20:00:00Z
updated: 2026-05-03T20:30:00Z
---

## Current Test

[all 3 manual tests passed — user reported 'all good']

## Tests

### 1. Bitwig host load — VST3 variant
expected: Plugin appears under vendor "Dukko Audio" as "Dukko" in Bitwig's plugin browser; instantiates on an empty audio track without error; Activity Monitor reports Bitwig as Apple (arm64), not Intel/Rosetta.
result: passed

### 2. Bitwig host load — CLAP variant
expected: CLAP variant appears under vendor "Dukko Audio" in Bitwig; instantiates on an empty audio track without error.
result: passed

### 3. README CI badge renders green at https://github.com/Aottens/Dukko
expected: "Build & Validate: passing" badge visible at the top of the README on the GitHub repo landing page.
result: passed

## Summary

total: 3
passed: 3
issues: 0
pending: 0
skipped: 0
blocked: 0

## Gaps
