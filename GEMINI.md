# Project Overview

This is a PlatformIO project for an ESP32-based custom DIY module designed for a cycling trainer. The firmware calculates cadence, power, and speed, and then transmits this data to a Garmin watch via Bluetooth Low Energy (BLE).

**Key Components:**
*   **Cadence:** Manages the calculation of cycling cadence based on reed switch pulses, including debouncing, timeout handling, and total revolution counting.
*   **Power:** Calculates power output based on cadence and resistance level using a lookup table (LUT) with specific interpolation and cutoff logic.
*   **ResistanceLevel:** Implements a state machine to manage and detect resistance levels, involving multiple pin inputs (backwards, forwards, position, limit pins) and a position change counter.
*   **BLECyclingPowerService:** Handles the Bluetooth Low Energy communication, exposing the cycling power service to compatible devices like Garmin watches.
*   **SystemWrapper:** Provides an abstraction layer for hardware-specific system calls (e.g., `millis()`, `digitalRead()`), facilitating unit testing.
*   **CyclingPowerGarmin:** The main application entry point (Arduino `setup()` and `loop()`).

**Technologies Used:**
*   **PlatformIO:** For project management, building, and testing.
*   **ESP32:** The target microcontroller.
*   **Arduino Framework:** The embedded development framework.
*   **NimBLE-Arduino:** A highly optimized BLE stack for ESP32.
*   **Unity:** A C testing framework for unit testing.

# Building and Running

## Building the Firmware

To build the firmware for the ESP32 target:

```bash
platformio run -e esp32doit-devkit-v1
```

## Uploading the Firmware

To upload the built firmware to the ESP32 device:

```bash
platformio run -t upload -e esp32doit-devkit-v1
```

## Running Tests

This project utilizes host-based unit testing with the Unity framework to ensure the correctness of core logic modules (Cadence, ResistanceLevel, Power).

To run all configured unit tests:

```bash
platformio test
```

This command will execute tests in the `native`, `test_resistance`, and `test_power` environments as defined in `platformio.ini`.

To run tests for a specific module, use the `-e` flag:

*   **Cadence Tests:**
    ```bash
    platformio test -e native
    ```
*   **Resistance Level Tests:**
    ```bash

    platformio test -e test_resistance
    ```
*   **Power Calculation Tests:**
    ```bash
    platformio test -e test_power
    ```

# Development Conventions

*   **PlatformIO:** The project is structured and managed using PlatformIO. Developers should be familiar with PlatformIO CLI commands and project structure.
*   **Arduino Framework:** Standard Arduino API calls are used within the main application logic and hardware interactions.
*   **Unit Testing (Unity):** Core logic modules are covered by unit tests using the Unity test framework. Tests are configured to run natively on the host machine for faster feedback cycles. When implementing new features or fixing bugs, new unit tests or updates to existing ones are expected.
*   **System Abstraction:** Hardware-dependent functions are abstracted through `ISystemWrapper` and `SystemWrapper` to enable easier mocking and testing in a native environment.
*   **Header-Only Power Logic:** The `Power` calculation logic, including its lookup table, is entirely defined within `Power.h`.
*   **Test File Duplication:** Due to current limitations and intricacies of PlatformIO's native testing environment for complex dependency trees, some source files (e.g., `Cadence.h/.cpp`, `ResistanceLevel.h/.cpp`, `SystemWrapper.h`) are duplicated into their respective test directories (e.g., `test/test_cadence_logic/`) and modified for native test compatibility (e.g., removing `#include <Arduino.h>`) rather than relying solely on `src/` includes. This ensures the tests build and run correctly without affecting the main firmware build.
