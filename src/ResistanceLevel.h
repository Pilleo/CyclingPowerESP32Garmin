#ifndef RESISTANCE_LEVEL_H
#define RESISTANCE_LEVEL_H

#include "SystemWrapper.h"

class ResistanceLevel
{
public:
    explicit ResistanceLevel(uint8_t backwardsPin, uint8_t limitPin, uint8_t positionPin, uint8_t forwardsPin, ISystemWrapper& sys);

    void update();
    auto getLevel() const -> uint8_t;
    auto getPositionChangeCounter() const -> uint16_t;
    auto level() -> uint8_t; // Legacy wrapper

private:
    ISystemWrapper& sys;
    const uint8_t limitPin;
    const uint8_t backwardsPin;
    const uint8_t positionPin;
    const uint8_t forwardsPin;

    bool oldPositionState;
    uint8_t currentLevel;
    uint32_t lastPulseTimestamp;

    // 'prevDirection' from original code
    bool prevDirectionWasForward;

    uint16_t positionChangeCounter;

    void updateLevelFromCounter();

    static constexpr uint8_t MIN_LEVEL = 1;
    static constexpr unsigned long DEBOUNCE_TIME_MS = 8;
    static constexpr unsigned long MOVEMENT_TIMEOUT_MS = 50;
    static constexpr unsigned long PAUSE_TIMEOUT_MS = 1700;
};

#endif // RESISTANCE_LEVEL_H