# Garmin Speed Recovery — Execution Record

## Goal

Keep power, cadence, resistance, and reconnection behavior intact; publish conforming CPS and CSC
telemetry; preserve fractional cadence in the power model; and be ready when Garmin restores support
for speed from this BLE configuration.

## Implemented

- NimBLE constraint remains `^2.5.0` and resolves to 2.5.1.
- Polling remains enabled (`USE_INTERRUPTS == 0`).
- Wheel-to-crank ratio is centralized at 3.
- CPS publishes power, wheel revolutions/time, and crank revolutions/time with Feature `0x0010000C`.
- CSC publishes wheel and crank data with Feature `0x0003`, Sensor Location, and mandatory SC Control
  Point support for Set Cumulative Value.
- Battery Service remains available.
- Production uses the ESP32 hardware BLE identity; the temporary diagnostic random address was removed.
- Fractional cadence reaches both the LED and `Power::calculate` without integer truncation.
- `Power.cpp` and its lookup table are outside this work.

## Hardware evidence

Test receiver: Garmin Fenix 7, firmware 26.09.

- CPS variants using ratios 3 and 31, feature values `0x0010000C`, `0x0000000C`, and wheel-torque
  context were all subscribed and produced power/cadence, while speed remained `--`.
- NimBLE 2.3.7 and 2.5.1 behaved the same.
- Changing CPS Sensor Location and including Battery Service did not restore speed.
- CSC-only firmware on the same hardware displayed cadence and speed.
- Combined CPS+CSC was discovered by Garmin as a power meter and did not expose a separately pairable
  speed/cadence role.

These results locate the remaining limitation in Garmin's current BLE sensor-role handling, not in
the transmitted revolution counters or timestamps.

## Hardware milestone

After uploading production firmware, verify power, cadence, resistance response, sleep/wake, and
reconnection. Speed may remain `--` on Fenix 7 firmware 26.09. When Garmin firmware changes, remove
and re-add the sensor, then verify live and recorded speed plus distance at 30, 60, and 90 RPM.
