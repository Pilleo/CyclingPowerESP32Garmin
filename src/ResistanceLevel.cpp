#include "ResistanceLevel.h"
#include <array>

// Internal Implementation Details
namespace {

    struct LevelRange {
        uint16_t minCount;
        uint16_t maxCount;
        uint8_t level;
    };

    // Ranges map the 'positionChangeCounter' ticks to a Resistance Level (1-16).
    constexpr std::array<LevelRange, 16> LEVEL_RANGES = {{
        {0,   29,    1},
        {170, 190,   2},
        {281, 308,   3},
        {360, 374,   4},
        {411, 429,   5},
        {461, 475,   6},
        {501, 519,   7},
        {531, 549,   8},
        {561, 575,   9},
        {591, 600,  10},
        {611, 626,  11},
        {631, 640,  12},
        {653, 660,  13},
        {666, 678,  14},
        {681, 689,  15},
        {692, 65535, 16}
    }};

} // namespace


ResistanceLevel::ResistanceLevel(const uint8_t backwardsPin, const uint8_t limitPin, const uint8_t positionPin,
                                 const uint8_t forwardsPin, ISystemWrapper& sys)
    : sys(sys), limitPin(limitPin), backwardsPin(backwardsPin), positionPin(positionPin), forwardsPin(forwardsPin)
{
    oldPositionState = (sys.digitalRead(positionPin) != 0);
    currentLevel = MIN_LEVEL;
    lastPulseTimestamp = 0;
    movingForward = true;
    positionChangeCounter = 0;
}

auto ResistanceLevel::level() -> uint8_t {
    update();
    return getLevel();
}

auto ResistanceLevel::getLevel() const -> uint8_t {
    return currentLevel;
}

auto ResistanceLevel::getPositionChangeCounter() const -> uint16_t {
    return positionChangeCounter;
}

void ResistanceLevel::checkResetCondition(bool isMovingForward) {
    const bool limitActive = (sys.digitalRead(limitPin) != 0);

    // If limit switch is hit and we are NOT moving forward, reset.
    if (!isMovingForward && limitActive) {
        positionChangeCounter = 0;
        currentLevel = MIN_LEVEL;
    }
}

void ResistanceLevel::updatePositionCounter(bool isMovingForward, bool isMovingBack, unsigned long deltaMs) {

    // Conditions used in original logic:
    // 1. Consistent Movement: time < 50ms
    // 2. Resume After Pause: time > 1700ms
    bool isFastMovement = (deltaMs < MOVEMENT_TIMEOUT_MS);
    bool isResumeAfterPause = (deltaMs > PAUSE_TIMEOUT_MS);

    // Logic:
    if (isMovingForward) {
        // Original: if (forward && ((isConsistentMovement && wasInPositiveDirection) || isMovementAfterLongPause))
        // wasInPositiveDirection returns prevDirection (movingForward).
        // So we check if PREVIOUS state was also forward.
        if ((isFastMovement && movingForward) || isResumeAfterPause) {
            positionChangeCounter++;
        }
    } else if (isMovingBack && positionChangeCounter > 0) {
        // Original: else if (counter > 0 && back && (isMovementAfterLongPause || (isConsistentMovement && !wasInPositiveDirection)))
        // !wasInPositiveDirection means previous state was NOT forward (False).
        if (isResumeAfterPause || (isFastMovement && !movingForward)) {
            positionChangeCounter--;
        }
    }

    // Update History (matches "prevDirection = forward" at end of block)
    // Note: If pins are Idle/Invalid (both false/true), isMovingForward is false,
    // so movingForward becomes false. This matches original behavior perfectly.
    movingForward = isMovingForward;
}

void ResistanceLevel::updateLevelFromCounter() {
    for (const auto& range : LEVEL_RANGES) {
        if (positionChangeCounter >= range.minCount && positionChangeCounter <= range.maxCount) {
            currentLevel = range.level;
            return;
        }
    }
}

void ResistanceLevel::update() {
    // 1. Read Inputs
    const bool pinBack = (sys.digitalRead(backwardsPin) != 0);
    const bool pinForward = (sys.digitalRead(forwardsPin) != 0);
    const bool pinPosition = (sys.digitalRead(positionPin) != 0);

    // Determine Logic State
    const bool isMovingBack = pinBack && !pinForward;
    const bool isMovingForward = pinForward && !pinBack;

    // 2. Handle Limit Reset
    checkResetCondition(isMovingForward);

    const unsigned long now = sys.millis();
    const unsigned long timeSinceLastPulse = now - lastPulseTimestamp;

    // 3. Detect Position Pulse (Debounced)
    if (pinPosition != oldPositionState && timeSinceLastPulse > DEBOUNCE_TIME_MS) {

        oldPositionState = pinPosition;

        // 4. Update Counter Logic
        // CRITICAL FIX: Pass 'timeSinceLastPulse' (delta) computed BEFORE updating lastPulseTimestamp.
        // We also pass isMovingBack to handle the strict logic flow.
        updatePositionCounter(isMovingForward, isMovingBack, timeSinceLastPulse);

        // 5. Update Timestamp (After logic uses the delta)
        lastPulseTimestamp = now;

        // 6. Map Counter to Level
        updateLevelFromCounter();
    }
}