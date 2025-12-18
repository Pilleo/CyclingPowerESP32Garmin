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
    explicit ResistanceLevel(uint8_t backwardsPin, uint8_t limitPin, uint8_t positionPin, uint8_t forwardsPin, ISystemWrapper& sys);

    void update();

    auto getLevel() const -> uint8_t;
    auto getPositionChangeCounter() const -> uint16_t;
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

    // Tracks the direction state of the *previous* pulse.
    // Equivalent to 'prevDirection' in the original code.
    bool movingForward;

    uint16_t positionChangeCounter;

    // --- Logic Helpers ---
    void checkResetCondition(bool isMovingForward);

    /**
     * @brief Updates the position counter based on direction and time delta.
     * @param isMovingForward Current pin state indicating forward.
     * @param isMovingBack Current pin state indicating backward.
     * @param deltaMs Time in milliseconds since the last pulse.
     */
    void updatePositionCounter(bool isMovingForward, bool isMovingBack, unsigned long deltaMs);

    void updateLevelFromCounter();

    // --- Constants ---
    static constexpr uint8_t MIN_LEVEL = 1;
    static constexpr uint8_t MAX_LEVEL = 16;
    static constexpr unsigned long DEBOUNCE_TIME_MS = 8;
    static constexpr unsigned long MOVEMENT_TIMEOUT_MS = 50;
    static constexpr unsigned long PAUSE_TIMEOUT_MS = 1700;
};

#endif // RESISTANCE_LEVEL_H