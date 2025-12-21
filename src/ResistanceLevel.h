#ifndef RESISTANCE_LEVEL_H
#define RESISTANCE_LEVEL_H

#include <cstdint>
#include "SystemWrapper.h"
#ifdef ESP32
#include "esp_attr.h"
#define ISR_ATTR IRAM_ATTR
#else
#define ISR_ATTR
#endif

struct ResistanceLevelPins {
    const uint8_t backwards;
    const uint8_t limit;
    const uint8_t position;
    const uint8_t forwards;
};

class ResistanceLevel {
public:
    explicit ResistanceLevel(const ResistanceLevelPins& pins, ISystemWrapper &sys) noexcept;

    void begin(); // Initialize hardware state

    auto getLevel() const -> uint8_t;

    auto getPositionChangeCounter() const -> uint16_t;

    auto level() const -> uint8_t; // // Wrapper returns level
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

    static void isrPosition() ISR_ATTR;

    // NEW: ISR for Limit Switch
    void handleLimitInterrupt();

    static void isrLimit() ISR_ATTR;

private:
    ISystemWrapper &sys;
    const uint8_t limitPin;
    const uint8_t backwardsPin;
    const uint8_t positionPin;
    const uint8_t forwardsPin;

    volatile bool oldPositionState;

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
    static ResistanceLevel *_instance;
};

#endif // RESISTANCE_LEVEL_H
