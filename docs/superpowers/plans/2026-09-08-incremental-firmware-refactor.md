# CyclingPowerGarmin Incremental Firmware Refactor

## Progress

- [x] 5% Checkpoint tested power and packet changes; create isolated worktree.
- [x] 20% Add typed cadence and telemetry contracts with characterization tests.
- [x] 45% Replace cadence sentinel behavior with a pure, timestamped polling model.
- [x] 65% Refactor resistance into a pure reducer after Milestone 1 hardware approval.
- [ ] 85% Separate BLE profiles after Milestone 2 hardware approval.
- [ ] 100% Simplify the coordinator and publish final documentation.

## Fixed decisions

- Polling remains the only supported production mode (`USE_INTERRUPTS == 0`).
- Interrupt handling is quarantined; it will not be expanded by this refactor.
- `CadenceReading` carries RPM, cumulative crank revolutions, BLE crank event
  time, and an explicit phase. `SleepReady` replaces the prior negative-RPM
  sentinel.
- `CyclingTelemetry` replaces positional BLE update arguments.
- Cadence event time is accumulated at accepted physical pulse timestamps,
  preserving modular `uint16_t` rollover even when the main loop is delayed.
- The power table, BLE UUIDs, wheel ratio, and Garmin compatibility behavior
  are preserved.

## Milestone 1

Before resistance or BLE structural work, upload this revision and test startup,
30/60/90 RPM cadence and power, stop/restart behavior, sleep/wake, CPS/CSC
counters, and Garmin reconnect behavior. Garmin Fenix 7 firmware 26.09 may
continue to show speed as `--`.

Milestone 1 cadence behavior passed on the trainer. Resistance diagnostics are
now staged behind Hardware Gate A before changing any counter behavior.
