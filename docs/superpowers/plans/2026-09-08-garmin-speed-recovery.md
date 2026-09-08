# Garmin Speed Recovery — Execution Record

## Goal

Keep power, cadence, resistance, and reconnection behavior intact; publish conforming CPS and CSC
telemetry; preserve fractional cadence in the power model; and restore Garmin speed compatibility.

## Implemented

- NimBLE constraint remains `^2.5.0` and resolves to 2.5.1.
- Polling remains enabled (`USE_INTERRUPTS == 0`).
- Wheel-to-crank ratio is centralized at 3.
- CPS publishes power, wheel revolutions/time, and crank revolutions/time with Feature `0x0010000C`.
- CPS exposes Cycling Power Control Point `2A66` and supports Set Cumulative Value without changing
  crank counters or event timestamps.
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
- Combined CPS+CSC without CPS Control Point was discovered by Garmin as a power meter but Garmin did
  not derive speed from its otherwise valid wheel data.
- In a controlled CPS-only comparison, the baseline without `2A66` showed power and cadence with speed
  `--`; the otherwise identical `2A66` variant showed 200 W, 60 RPM, and 22.6 km/h. Stop/run behavior,
  saved speed, and fresh pairing were verified on the Fenix 7.

These results isolate the compatibility requirement to CPS Control Point availability when wheel
revolution data is advertised. The January receiver update exposed the omission; its internal Garmin
validation behavior remains undocumented.

## Hardware milestone

After uploading production firmware, remove and re-add the sensor, then verify live and saved power,
cadence, speed, distance, auto-pause, resistance response, sleep/wake, and reconnection at 30, 60,
and 90 RPM with USB disconnected.
