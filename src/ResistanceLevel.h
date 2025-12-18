#ifndef RESISTANCE_LEVEL_H
#define RESISTANCE_LEVEL_H

#include <cstdint>
#include "SystemWrapper.h"

/**
 * @brief Manages the resistance level detection based on reed switch position encoding.
 */
class ResistanceLevel
{
public:
    /**
     * @brief Construct a new Resistance Level object
     *
     * @param backwardsPin Pin indicating backward movement
     * @param limitPin Pin indicating the physical limit (reset) switch
     * @param positionPin Pin giving pulses for position increments
     * @param forwardsPin Pin indicating forward movement
     * @param sys System wrapper for hardware abstraction
     */
    explicit ResistanceLevel(uint8_t backwardsPin, uint8_t limitPin, uint8_t positionPin, uint8_t forwardsPin, ISystemWrapper& sys);

    /**
     * @brief Main processing loop. Reads sensors and updates internal state.
     * Should be called frequently in the main loop.
     */
    void update();

    /**
     * @brief Gets the currently calculated resistance level.
     * @return uint8_t Level (1-16)
     */
    auto getLevel() const -> uint8_t;

    /**
     * @brief Returns the raw internal position counter (useful for debugging/calibration).
     */
    auto getPositionChangeCounter() const -> uint16_t;

    /**
     * @brief Legacy wrapper for backward compatibility.
     * Calls update() and returns getLevel().
     */
    auto level() -> uint8_t;

private:
    ISystemWrapper& sys;
    const uint8_t limitPin;
    const uint8_t backwardsPin;
    const uint8_t positionPin;
    const uint8_t forwardsPin;

    bool oldPositionState;
    uint8_t currentLevel;
    uint32_t lastPulseTimestamp;
    bool movingForward; // true = forward, false = backward
    uint16_t positionChangeCounter;

    // --- Logic Helpers ---
    void checkResetCondition(bool isMovingForward);
    void updatePositionCounter(bool isMovingForward,bool isMovingBack, unsigned long now);
    void updateLevelFromCounter();

    // --- Constants ---
    static constexpr uint8_t MIN_LEVEL = 1;
    static constexpr uint8_t MAX_LEVEL = 16;
    static constexpr unsigned long DEBOUNCE_TIME_MS = 8;
    static constexpr unsigned long MOVEMENT_TIMEOUT_MS = 50;
    static constexpr unsigned long PAUSE_TIMEOUT_MS = 1700;
};

#endif // RESISTANCE_LEVEL_H