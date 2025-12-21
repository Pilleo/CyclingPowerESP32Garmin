#ifndef CADENCE_H
#define CADENCE_H

#include <cstdint>
#include "SystemWrapper.h"
// ESP32 specific attribute for ISRs to run from RAM (faster/safer)
#if defined(ESP32)
    #include "esp_attr.h"
    #define ISR_ATTR IRAM_ATTR
#else
    #define ISR_ATTR
#endif
class Cadence
{
public:
    explicit Cadence(uint8_t pinn, ISystemWrapper& sys) noexcept;

    void begin(); // Initialize hardware state

    /**
     * @brief Polling Driver.
     * Checks the physical pin state. If a pulse is detected, triggers onPulse().
     * Call this in the main loop if NOT using interrupts.
     */
    void poll();

    /**
     * @brief Calculates and returns the current Cadence (RPM).
     * Does NOT read hardware pins. Uses internal state.
     */
    auto cadence() -> float;

    auto lastTimestamp() const -> uint32_t;
    auto getGattLastCrankRevolutionTimestamp() const -> uint16_t;
    auto totalRevs() const -> uint32_t;

    // Core Logic (Public for Tests/ISRs)
    void onPulse(uint32_t timestampMs);

    // NEW: ISR Entry Points
    // Sets the static instance pointer to 'this'
    void enableInterrupt();

    // The actual code to run inside the ISR (Instance context)
    void ISR_ATTR handleInterrupt();

    // The static function to attach to the microcontroller (Global context)
    static void isr();

private:
    ISystemWrapper& sys;
    const uint8_t pin;
    float rpm = 0;

    uint32_t lastIntervalTime = 0;
    bool oldState; // Used only by poll()

    volatile uint32_t elapsedTimestamp = 0;
    volatile uint32_t elapsedSampleTimeStamp = 0;
    volatile uint8_t rev = 0;
    volatile uint32_t totalRev = 0;
    volatile uint16_t gattLastCrankRevolutionTimestamp = 0;

    static constexpr uint32_t MIN_DELAY_BETWEEN_FULL_ROTATION_MS = 330;
    static constexpr uint32_t MAX_IDLE_TIMEOUT_MS = 5000;
    static constexpr uint32_t DEEP_SLEEP_TIMEOUT_MS = 60000 * 15;
    // NEW: Static pointer to the active instance
    static Cadence* _instance;
};

#endif //CADENCE_H