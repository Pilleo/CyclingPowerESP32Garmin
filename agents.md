# CyclingPowerGarmin

**CyclingPowerGarmin** is an ESP32 firmware project designed to convert a standard exercise bike
(with 16 steps of spinning resistance) into a smart trainer compatible with Garmin watches and 
other BLE Cycling Power Profile (CPP) receivers.
It is not an FTMS project as Garmin still does not support the standard, so it is axposed as a regular cycling power meter.
The system estimates **Power (Watts)** via a software model that correlates **Cadence (RPM)** and **Resistance Level** (derived from a mechanical position sensor) using a lookup table.
Using polling currently as interrupts did not work great with resistance with current implementation
## Architecture & Design

The firmware is designed with **Testability** as a core principle. It decouples hardware interactions from business logic using Dependency Injection.

### Key Components

*   **`BikeComputer`**: The central controller. It orchestrates the flow of data between sensors (Cadence, Resistance), the calculation engine (Power), and the output interface (BLE).
*   **`Cadence`**: Calculates RPM based on pulses from a magnetic reed switch or Hall sensor. Includes logic for debouncing, idle timeouts, and coasting decay.
*   **`ResistanceLevel`**: Determines the current resistance setting (1-16) by tracking the movement of the resistance cable/magnet. It uses a state machine monitoring 4 pins (Forward, Backward, Position Pulse, Limit Switch).
*   **`Power`**: A pure logic module containing a Lookup Table (LUT). It accepts `Level` and `Cadence` to return estimated `Watts`, interpolating values where necessary.
*   **`BLECyclingPowerService`**: Manages the Bluetooth Low Energy stack (using **NimBLE**). It broadcasts the *Cycling Power Service* (0x1818) and notifies connected devices of power and crank revolution data.
*   **`ISystemWrapper`**: An abstract interface for system calls (`millis`, `digitalRead`, `digitalWrite`, `deepSleep`).
    *   **`ArduinoSystemWrapper`**: The concrete implementation used on the ESP32.
    *   **`MockSystemWrapper`**: A mock implementation used during Native Unit Testing to simulate time and pin states.

## Hardware Configuration

The project targets an **ESP32** (specifically configured for `lolin32_lite` in `platformio.ini`).

### Pinout (Default)

Defined in `src/CyclingPowerGarmin.cpp` and `src/BikeComputer.h`:

| Component | Pin (GPIO) | Description |
| :--- | :--- | :--- |
| **Cadence Sensor** | **15** | Input. Reed switch/Hall sensor. Active Low/High depending on magnet. |
| **Resistance Back** | **4** | Input. Signal that resistance is decreasing. |
| **Resistance Fwd** | **19** | Input. Signal that resistance is increasing. |
| **Resistance Pos** | **23** | Input. Pulse pin; toggles as resistance knob turns. |
| **Resistance Limit** | **18** | Input. Limit switch to recalibrate level to 1. |
| **LED** | **27** | Output. Status LED (Blinks based on RPM). |
| **Wakeup** | **15** | Deep sleep wakeup source (tied to Cadence). |

## Development & Testing

This project uses **PlatformIO**. It can be istalled like this:
```bash
curl -fsSL -o /tmp/get-platformio.py https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py
python3 /tmp/get-platformio.py
export PATH=$PATH:$HOME/.local/bin
```

To run tests with coverage lcov also needs to be installed:
```bash
sudo apt install lcov -y
```

### 1. Build & Upload (Firmware)

To compile, test, flash the code to the ESP32:

```bash
# Build
pio run -e lolin32_lite

# Upload
pio run -t upload -e lolin32_lite

# Monitor Serial Output
pio run -t monitor -e lolin32_lite

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