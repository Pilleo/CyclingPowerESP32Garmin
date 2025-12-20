#ifndef RESISTANCE_LEVEL_H
#define RESISTANCE_LEVEL_H

#include <cstdint>
#include "SystemWrapper.h"
#if defined(ESP32)
    #include "esp_attr.h"
    #define ISR_ATTR IRAM_ATTR
#else
    #define ISR_ATTR
#endif
class ResistanceLevel
{
public:
    explicit ResistanceLevel(uint8_t backwardsPin, uint8_t limitPin, uint8_t positionPin, uint8_t forwardsPin, ISystemWrapper& sys);

    auto getLevel() const -> uint8_t;
    auto getPositionChangeCounter() const -> uint16_t;
    auto level() -> uint8_t; // // Wrapper returns level
    /**
      * @brief Polling Driver.
      * Reads all resistance pins and triggers events if edges are detected.
      */
    void poll();

    /**
     * @brief Triggered when the position sensor detects an edge (pulse).
     * Handles debouncing, direction logic, and counter updates.
     *
     * @param timestampMs Current time in milliseconds.
     * @param isMovingForward True if the Forward pin signals movement.
     * @param isMovingBack True if the Backward pin signals movement.
     */
    void onPositionPulse(uint32_t timestampMs, bool isMovingForward, bool isMovingBack);

    /**
     * @brief Triggered when the limit switch is active and movement is not forward.
     * Resets calibration.
     */
    void onLimitReset();

    // NEW: ISR Entry Points
    void enableInterrupt();
    void handlePositionInterrupt();
    static void ISR_ATTR isrPosition();

private:
    ISystemWrapper& sys;
    const uint8_t limitPin;
    const uint8_t backwardsPin;
    const uint8_t positionPin;
    const uint8_t forwardsPin;

    bool oldPositionState;

    // Volatile: Accessed by future ISRs
    volatile uint8_t currentLevel;
    volatile uint32_t lastPulseTimestamp;
    volatile bool prevDirectionWasForward;
    volatile uint16_t positionChangeCounter;

    void updateLevelFromCounter();

    static constexpr uint8_t MIN_LEVEL = 1;
    static constexpr unsigned long DEBOUNCE_TIME_MS = 8;
    static constexpr unsigned long MOVEMENT_TIMEOUT_MS = 50;
    static constexpr unsigned long PAUSE_TIMEOUT_MS = 1700;
    // NEW: Static pointer
    static ResistanceLevel* _instance;
};

#endif // RESISTANCE_LEVEL_H