#ifndef CADENCE_H
#define CADENCE_H

#include <cstdint>
#include "SystemWrapper.h"

class Cadence
{
public:
    explicit Cadence(uint8_t pinn, ISystemWrapper& sys);

    auto cadence() -> float;

    auto lastTimestamp() const -> uint32_t;

    auto getGattLastCrankRevolutionTimestamp() const -> uint16_t;

    auto totalRevs() const -> uint32_t;

    /**
     * @brief Triggered when a magnet pulse is detected.
     * Handles debouncing and counter incrementing.
     *
     * @param timestampMs The time (millis) when the pulse occurred.
     */
    void onPulse(uint32_t timestampMs);

private:

    ISystemWrapper& sys;
    const uint8_t pin;
    float rpm = 0;

    uint32_t lastIntervalTime = 0;
    bool oldState;

    // Volatile: These will be accessed by ISRs in the future
    volatile uint32_t elapsedTimestamp = 0;
    volatile uint32_t elapsedSampleTimeStamp = 0;

    volatile uint8_t rev = 0;
    volatile uint32_t totalRev = 0;
    volatile uint16_t gattLastCrankRevolutionTimestamp = 0;

    static constexpr uint32_t MIN_DELAY_BETWEEN_FULL_ROTATION_MS = 330;
    static constexpr uint32_t MAX_IDLE_TIMEOUT_MS = 5000;
    static constexpr uint32_t DEEP_SLEEP_TIMEOUT_MS = 60000 * 15;
};

#endif //CADENCE_H