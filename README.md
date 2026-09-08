# CyclingPowerGarmin

**CyclingPowerGarmin** is an ESP32 firmware project designed to convert a standard exercise bike
(with 16 steps of spinning resistance) into a smart trainer compatible with Garmin watches and
other BLE Cycling Power Profile (CPP) receivers.
It is not an FTMS project as Garmin currently does not support the standard, so it is exposed as a regular cycling power
meter.
The system estimates **Power (Watts)** via a software model that correlates **Cadence (RPM)** and **Resistance Level** (
derived from a mechanical position sensor) using a lookup table.
The system uses **polling** in production (`USE_INTERRUPTS 0` in `config.h`). Interrupt code remains experimental and is not hardware-validated for this trainer.
This firmware is working with a trainer that has built-in sensors (Hall/Reed switches) typically wired to a basic cycling computer.

## Architecture & Design

The firmware is designed with **Testability** as a core principle. It decouples hardware interactions from business
logic using Dependency Injection.

### Key Components

* **`BikeComputer`**: The central controller. It orchestrates the flow of data between sensors (Cadence, Resistance),
  the calculation engine (Power), and the output interface (BLE).
* **`Cadence`**: Calculates RPM based on pulses from a magnetic reed switch or Hall sensor. Includes logic for
  debouncing, idle timeouts, and coasting decay.
* **`ResistanceLevel`**: Determines the current resistance setting (1-16) by tracking the movement of the resistance
  cable/magnet. It uses a state machine monitoring 4 pins (Forward, Backward, Position Pulse, Limit Switch) through the validated polling path.
* **`Power`**: A pure logic module containing a 2D Lookup Table (LUT). It accepts `Level` and `Cadence` to return estimated
  `Watts`, performing bilinear interpolation for precise values.
* **`BLECyclingPowerService`**: Manages the Bluetooth Low Energy stack. It exposes both Cycling
  Power (`0x1818`) and Cycling Speed and Cadence (`0x1816`), plus Battery Service (`0x180F`).
  CPS and CSC are generated from the same revolution counter and event timestamp. CPS exposes
  Cycling Power Control Point (`0x2A66`) and CSC exposes the mandatory SC Control Point (`0x2A55`).
    * Uses **`BlePacketGenerator`** and **`CscPacketGenerator`** to format valid BLE packets.
* **`IBleStackAdapter`**: An abstraction over the BLE stack to facilitate testing.
    *   **`NimBleStackAdapter`**: Concrete implementation wrapping the **NimBLE** library for ESP32.
* **`ISystemWrapper`**: An abstract interface for system calls (`millis`, `digitalRead`, `digitalWrite`, `deepSleep`).
    * **`ArduinoSystemWrapper`**: The concrete implementation used on the ESP32.
    * **`MockSystemWrapper`**: A mock implementation used during Native Unit Testing to simulate time and pin states.

## Hardware Configuration

The project targets an **ESP32** (specifically configured for `lolin32_lite` in `platformio.ini`).

### Pinout (Default)

Defined in `src/config.h` and `src/BikeComputer.h`:

| Component            | Pin (GPIO) | Description                                                                          |
|:---------------------|:-----------|:-------------------------------------------------------------------------------------|
| **Cadence Sensor**   | **15**     | Input. Reed switch/Hall sensor. Active Low/High depending on magnet.                 |
| **Resistance Back**  | **4**      | Input. Signal that resistance is decreasing. Comes from a motor adjusting resistance |
| **Resistance Fwd**   | **19**     | Input. Signal that resistance is increasing. Comes from a motor adjusting resistance |
| **Resistance Pos**   | **23**     | Input. Pulse pin; toggles as resistance knob turns. Comes from an optical sensor     |
| **Resistance Limit** | **18**     | Input. Limit switch to recalibrate level to 1.                                       |
| **LED**              | **27**     | Output. Status LED (Blinks based on RPM).                                            |
| **Wakeup**           | **15**     | Deep sleep wakeup source (tied to Cadence).                                          |

## Development & Testing

### BLE compatibility

The NimBLE dependency remains `h2zero/NimBLE-Arduino@^2.5.0`; the verified resolution is 2.5.1.
The configured wheel-to-crank ratio is 3. With a receiver configured for a 2.1 m circumference,
speed is `cadence × 3 × 2.1 × 60 / 1000`, giving approximately 11.3, 22.7, and 34.0 km/h at
30, 60, and 90 RPM. CPS Feature is `0x0010000C` (wheel and crank revolution data supported;
not for use in a distributed system).

Garmin Fenix 7 firmware 26.09 displayed power and cadence but speed `--` when CPS Wheel Revolution
Data was present without CPS Control Point (`0x2A66`). A controlled A/B test showed that adding `2A66`
restored live speed, saved speed, distance, and stop/run behavior. The result was reproduced after a
fresh pairing and then verified on the real trainer.

When CPS Wheel Revolution Data is supported, the CPS Set Cumulative Value procedure is conditionally
required. This firmware supports that procedure through `0x2A66` while preserving the crank counters
and event timestamps. The controlled test isolates that characteristic as the compatibility difference;
Garmin's internal validation behavior is undocumented. CSC remains available for receivers that use it,
although Garmin may still present the combined peripheral as a single power meter.

This project uses **PlatformIO**. It can be installed like this:

```bash
curl -fsSL -o /tmp/get-platformio.py https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py
python3 /tmp/get-platformio.py
export PATH=$PATH:$HOME/.local/bin
```

To run tests with coverage lcov also needs to be installed:

```bash
sudo apt install lcov -y
```

### 1. Build, Upload, and Pair

To compile, test, flash the code to the ESP32:

```bash
# Native tests
pio test -e native

# Build
pio run -e lolin32_lite

# Upload
pio run -t upload -e lolin32_lite

# Monitor Serial Output
pio run -t monitor -e lolin32_lite
```

After an upgrade that changes the BLE GATT layout, remove and re-add `CX6` on Garmin so it discovers
the new characteristics. Configure a 2100 mm wheel circumference when that matches your setup. Perform
trainer movement tests with USB disconnected: USB connection was observed to interfere with the trainer
electronics during resistance adjustment.

### Virtual speed

The virtual speed is derived from cadence and the configured wheel-to-crank ratio:

```text
speed_kmh = cadence_rpm × 3 × circumference_m × 0.06
```

At 60 RPM and 2100 mm circumference, this is 22.68 km/h. It is intended to provide stable recording
and auto-pause behavior. Matching a trainer's proprietary speed display exactly requires separate
calibration measurements.

### Coverage report

```bash

# Test with coverage
# 1. Clean previous builds to ensure fresh coverage data
pio run -e native -t clean

# 2. Run the tests in the native environment
pio test -e native

# 3. Capture coverage data (creates a tracefile)
lcov --capture --directory .pio/build/native/ --output-file coverage.info

# 4. Filter out system libraries and test framework code (optional but recommended)
lcov --remove coverage.info '/usr/*' '*/.pio/*' '*/test/*' --output-file coverage_filtered.info

# 5. Generate the HTML report
genhtml coverage_filtered.info --output-directory coverage_report
```
