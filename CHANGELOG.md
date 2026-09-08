# Changelog

## [Unreleased]

- Restored Garmin speed reporting by exposing Cycling Power Control Point (`2A66`) and supporting Set Cumulative Value.
- Publish Cycling Power and Cycling Speed and Cadence data from one telemetry snapshot.
- Preserve fractional cadence in calculated power.
- Model cadence and resistance state with typed, testable polling logic.
- Add packet, rollover, resistance, Control Point, BLE protocol, and integration regression tests.
