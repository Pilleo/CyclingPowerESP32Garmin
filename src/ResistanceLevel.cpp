#include "ResistanceLevel.h"
#include <array>

// Internal Implementation Details
namespace {

    struct LevelRange {
        uint16_t minCount;
        uint16_t maxCount;
        uint8_t level;
    };

    // Derived from original logic.
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
    // Pins are assumed to be initialized (INPUT_PULLUP) by caller or SystemWrapper
    oldPositionState = (sys.digitalRead(positionPin) != 0);
    currentLevel = MIN_LEVEL;
    lastPulseTimestamp = 0;
    movingForward = true;
    positionChangeCounter = 0;
}

// Legacy wrapper
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

    // If moving backwards (or idle) and hitting the limit switch, reset position.
    // NOTE: This relies on limitPin being reliable.
    if (!isMovingForward && limitActive) {
        positionChangeCounter = 0;
        currentLevel = MIN_LEVEL;
    }
}

void ResistanceLevel::updatePositionCounter(bool isMovingForward, bool isMovingBack, unsigned long now) {
    const unsigned long timeSinceLastPulse = now - lastPulseTimestamp;

    // Helper lambdas for readability
    auto isFastMovement = [&]() { return timeSinceLastPulse < MOVEMENT_TIMEOUT_MS; };
    auto isResumeAfterPause = [&]() { return timeSinceLastPulse > PAUSE_TIMEOUT_MS; };

    // Direction Consistency Check:
    // Only count pulses if the current direction matches the previous direction state.
    // 'movingForward' stores the state from the PREVIOUS pulse cycle.
    bool directionIsStable = (movingForward == isMovingForward);

    if (isMovingForward) {
        // Increment Condition:
        // 1. Consistent fast forward movement
        // 2. Or starting to move forward after a long pause
        if ((isFastMovement() && directionIsStable) || isResumeAfterPause()) {
            positionChangeCounter++;
        }
        // Update direction history (we are definitely moving forward)
        movingForward = true;

    } else if (isMovingBack) {
        // Decrement Condition:
        // Must check directionIsStable against the PREVIOUS state.
        // If we were moving Forward previously, movingForward=true.
        // Now isMovingBack=true.
        // We need to know if we were previously moving Backward.
        // If movingForward was true, then we CHANGED direction.

        // Let's refine directionIsStable logic for backward:
        // if movingForward was TRUE, then we are NOT stable in backward motion.
        // if movingForward was FALSE, then we MIGHT be stable (assuming we were backing before).

        bool prevWasBack = !movingForward;

        if (positionChangeCounter > 0) {
            if (isResumeAfterPause() || (isFastMovement() && prevWasBack)) {
                positionChangeCounter--;
            }
        }
        // Update direction history (we are definitely moving backward)
        movingForward = false;
    }

    // If neither is moving (Idle/Glitch), we do NOT update the counter or the direction history.
}

void ResistanceLevel::updateLevelFromCounter() {
    // Iterate through the ranges to find a match.
    // If current counter is in a "gap" (dead zone), we do NOT update currentLevel.
    for (const auto& range : LEVEL_RANGES) {
        if (positionChangeCounter >= range.minCount && positionChangeCounter <= range.maxCount) {
            currentLevel = range.level;
            return; // Found match, exit early
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
    // Note: We pass isMovingForward to ensure we don't reset while intentionally moving up.
    checkResetCondition(isMovingForward);

    const unsigned long now = sys.millis();
    const unsigned long timeSinceLastPulse = now - lastPulseTimestamp;

    // 3. Detect Position Pulse (Debounced)
    if (pinPosition != oldPositionState && timeSinceLastPulse > DEBOUNCE_TIME_MS) {

        // State changed, register the pulse
        oldPositionState = pinPosition;
        lastPulseTimestamp = now;

        // 4. Update Counter Logic
        // STRICT CHECK: Only update if strictly one direction is active.
        if (isMovingForward || isMovingBack) {
            updatePositionCounter(isMovingForward, isMovingBack, now);
        }

        // 5. Map Counter to Level
        updateLevelFromCounter();
    }
}