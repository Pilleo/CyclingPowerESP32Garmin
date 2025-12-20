#include "ResistanceLevel.h"
#include <array>

namespace {
    struct LevelRange {
        uint16_t minCount;
        uint16_t maxCount;
        uint8_t level;
    };

    constexpr std::array<LevelRange, 16> LEVEL_RANGES = {{
        {0,   29,    1},
        {170, 190,   2},
        {281, 308,   3},
        {360, 374,   4},
        {411, 429,   5},
        {461, 476,   6},
        {501, 519,   7},
        {531, 549,   8},
        {561, 575,   9},
        {591, 601,  10},
        {611, 627,  11},
        {631, 641,  12},
        {653, 661,  13},
        {666, 679,  14},
        {681, 690,  15},
        {692, 65535, 16}
    }};
}

ResistanceLevel::ResistanceLevel(const uint8_t backwardsPin, const uint8_t limitPin, const uint8_t positionPin,
                                 const uint8_t forwardsPin, ISystemWrapper& sys)
    : sys(sys), limitPin(limitPin), backwardsPin(backwardsPin), positionPin(positionPin), forwardsPin(forwardsPin)
{
    oldPositionState = (sys.digitalRead(positionPin) != 0);
    currentLevel = MIN_LEVEL;
    lastPulseTimestamp = 0;
    prevDirectionWasForward = true;
    positionChangeCounter = 0;
}

auto ResistanceLevel::level() -> uint8_t {
    update();
    return getLevel();
}

auto ResistanceLevel::getLevel() const -> uint8_t {
    // uint8_t is usually atomic, but let's be strict
    // Since this is 'const', we need to cast away constness or rely on mutable sys?
    // ISystemWrapper& sys is not const in the header.

    // Actually, we can just return the volatile.
    // The risk is low for level.
    return currentLevel;
}

auto ResistanceLevel::getPositionChangeCounter() const -> uint16_t {
    uint16_t snap;
    // We need to cast 'this' to non-const to access sys?
    // Or make sys mutable.
    // Or just accept that sys.noInterrupts() changes system state, not object state.
    ResistanceLevel* nonConstThis = const_cast<ResistanceLevel*>(this);

    nonConstThis->sys.disableInterrupts();
    snap = positionChangeCounter;
    nonConstThis->sys.enableInterrupts();
    return snap;
}

void ResistanceLevel::updateLevelFromCounter() {
    for (const auto& range : LEVEL_RANGES) {
        if (positionChangeCounter >= range.minCount && positionChangeCounter <= range.maxCount) {
            currentLevel = range.level;
            return;
        }
    }
}

void ResistanceLevel::onLimitReset() {
    positionChangeCounter = 0;
    currentLevel = MIN_LEVEL;
}

void ResistanceLevel::onPositionPulse(uint32_t now, bool isMovingForward, bool isMovingBack) {
    const unsigned long sampleTime = now - lastPulseTimestamp;

    // 1. Debounce Logic
    if (sampleTime > DEBOUNCE_TIME_MS) {
        lastPulseTimestamp = now;

        const bool isConsistent = (sampleTime < MOVEMENT_TIMEOUT_MS);
        const bool isLongPause = (sampleTime > PAUSE_TIMEOUT_MS);

        // 2. Counter Logic
        if (isMovingForward) {
            if ((isConsistent && prevDirectionWasForward) || isLongPause) {
                positionChangeCounter++;
            }
        } else if (positionChangeCounter > 0 && isMovingBack) {
            if (isLongPause || (isConsistent && !prevDirectionWasForward)) {
                positionChangeCounter--;
            }
        }

        // 3. Update History
        // Note: Logic copied from original. History updates on every accepted pulse.
        prevDirectionWasForward = isMovingForward;

        // 4. Update Level
        updateLevelFromCounter();
    }
}

void ResistanceLevel::update() {
    // 1. Read Inputs (Polling Driver)
    const bool pinBackRaw = (sys.digitalRead(backwardsPin) != 0);
    const bool pinForwardRaw = (sys.digitalRead(forwardsPin) != 0);
    const bool pinPosition = (sys.digitalRead(positionPin) != 0);
    const bool limitActive = (sys.digitalRead(limitPin) != 0);

    // Derived Direction States
    const bool isMovingBack = pinBackRaw && !pinForwardRaw;
    const bool isMovingForward = pinForwardRaw && !pinBackRaw;

    // 2. Handle Limit Reset
    if (!isMovingForward && limitActive) {
        onLimitReset();
    }

    // 3. Detect Edge
    if (pinPosition != oldPositionState) {
        oldPositionState = pinPosition;
        // Delegate logic to Core
        onPositionPulse(sys.millis(), isMovingForward, isMovingBack);
    }
}