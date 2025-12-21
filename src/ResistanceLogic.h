#ifndef RESISTANCE_LOGIC_H
#define RESISTANCE_LOGIC_H

#include <cstdint>
#include "SystemWrapper.h"
#include "drivers/ISensorDriver.h"

// Pins that are read directly by the logic class
struct ResistanceLogicPins {
    const uint8_t backwards;
    const uint8_t limit;
    const uint8_t forwards;
};

class ResistanceLogic {
public:
    explicit ResistanceLogic(const ResistanceLogicPins& pins, ISensorDriver& positionDriver, ISystemWrapper& sys) noexcept;

    void begin();
    void update(); // Replaces poll() and ISR handling

    auto level() const -> uint8_t;
    auto getPositionChangeCounter() const -> uint16_t;

private:
    void handlePositionPulse(uint32_t timestampMs, bool isMovingForward, bool isMovingBack);
    void onLimitReset();
    void updateLevelFromCounter();

    ISensorDriver& _positionDriver;
    ISystemWrapper& _sys;
    const uint8_t _limitPin;
    const uint8_t _backwardsPin;
    const uint8_t _forwardsPin;

    uint32_t _lastKnownEventCount;

    // State variables, no longer volatile as they are not accessed from ISRs
    uint8_t _currentLevel;
    uint32_t _lastPulseTimestamp;
    bool _prevDirectionWasForward;
    uint16_t _positionChangeCounter;

    static constexpr uint8_t MIN_LEVEL = 1;
    static constexpr unsigned long DEBOUNCE_TIME_MS = 8;
    static constexpr unsigned long MOVEMENT_TIMEOUT_MS = 50;
    static constexpr unsigned long PAUSE_TIMEOUT_MS = 1700;
};

#endif // RESISTANCE_LOGIC_H
