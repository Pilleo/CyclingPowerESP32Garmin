#ifndef CONFIG_H
#define CONFIG_H

#include <cstdint>

// Pin definitions
namespace config {
    // LED
    constexpr uint8_t LED_PIN = 27;

    // Cadence Sensor
    constexpr uint8_t CADENCE_PIN = 15;

    // Resistance Sensor
    constexpr uint8_t RESISTANCE_BACKWARDS_PIN = 4;
    constexpr uint8_t RESISTANCE_FORWARDS_PIN = 19;
    constexpr uint8_t RESISTANCE_POSITION_PIN = 23;
    constexpr uint8_t RESISTANCE_LIMIT_PIN = 18;

    // Wakeup
    constexpr uint8_t WAKEUP_PIN = CADENCE_PIN;
}

#endif // CONFIG_H
