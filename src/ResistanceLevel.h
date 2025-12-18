#include <cstdint>
#include "SystemWrapper.h"

class ResistanceLevel
{
public:
    explicit ResistanceLevel(uint8_t backwardsPin, uint8_t limitPin, uint8_t positionPin, uint8_t forwardsPin, ISystemWrapper& sys);

    auto level() -> uint8_t;
    auto getPositionChangeCounter() const -> uint16_t;

private:
    ISystemWrapper& sys;
    const uint8_t limittPin;
    const uint8_t backwardsPin;
    const uint8_t positionPin;
    const uint8_t forwardsPin;
    bool oldState;
    uint8_t currentLevel;
    uint32_t elapsedSampleTimeForLevel;
    bool prevDirection;
    uint16_t positionChangeCounter;

    void isFirstLevel(bool back);
    auto wasInPositiveDirection() const -> bool;
    static auto isConsistentMovement(unsigned long sampleTime) -> bool;
    static auto isMovementAfterLongPause(unsigned long sampleTime) -> bool;
};
