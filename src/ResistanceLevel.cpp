#include "ResistanceLevel.h"
#include <algorithm>
#include <array>

ResistanceLevel *ResistanceLevel::_instance = nullptr;

namespace {
struct LevelRange {
    uint16_t minCount;
    uint16_t maxCount;
    uint8_t level;
};

constexpr std::array<LevelRange, 16> LEVEL_RANGES = {
    {{0, 29, 1},
     {170, 190, 2},
     {281, 308, 3},
     {360, 374, 4},
     {411, 429, 5},
     {461, 476, 6},
     {501, 519, 7},
     {531, 549, 8},
     {561, 575, 9},
     {591, 601, 10},
     {611, 627, 11},
     {631, 641, 12},
     {653, 661, 13},
     {666, 679, 14},
     {681, 690, 15},
     {692, 65535, 16}}};
} // namespace

ResistanceLevel::ResistanceLevel(const ResistanceLevelPins& pins, ISystemWrapper &sys) noexcept
    : sys(sys),
      limitPin(pins.limit),
      backwardsPin(pins.backwards),
      positionPin(pins.position),
      forwardsPin(pins.forwards),
      oldPositionState(false),
      currentLevel(MIN_LEVEL),
      lastPulseTimestamp(0),
      prevDirectionWasForward(true),
      positionChangeCounter(0) {
}

void ResistanceLevel::begin() {
    oldPositionState = (sys.digitalRead(positionPin) != 0);
}

void ResistanceLevel::enableInterrupt() {
    _instance = this;
}

void ResistanceLevel::isrPosition() {
    if (_instance != nullptr) {
        _instance->handlePositionInterrupt();
    }
}

void ResistanceLevel::handlePositionInterrupt() {
    _lastPositionInterruptTime = sys.millis();
    _positionInterruptFlag = true;
}

void ResistanceLevel::isrLimit() {
    if (_instance != nullptr) {
        _instance->handleLimitInterrupt();
    }
}

void ResistanceLevel::handleLimitInterrupt() {
    _limitInterruptFlag = true;
}

void ResistanceLevel::update() {
    if (_limitInterruptFlag) {
        sys.disableInterrupts();
        _limitInterruptFlag = false;
        sys.enableInterrupts();

        const bool pinFwd = (sys.digitalRead(forwardsPin) != 0);
        const bool pinBack = (sys.digitalRead(backwardsPin) != 0);
        const bool isMovingForward = pinFwd && !pinBack;

        if (!isMovingForward && sys.digitalRead(limitPin) != 0) {
            onLimitReset();
        }
    }

    if (_positionInterruptFlag) {
        uint32_t interruptTime;
        sys.disableInterrupts();
        _positionInterruptFlag = false;
        interruptTime = _lastPositionInterruptTime;
        sys.enableInterrupts();

        const bool currentPos = (sys.digitalRead(positionPin) != 0);
        if (currentPos != oldPositionState) {
            oldPositionState = currentPos;

            const bool pinBack = (sys.digitalRead(backwardsPin) != 0);
            const bool pinFwd = (sys.digitalRead(forwardsPin) != 0);
            const bool isMovingBack = pinBack && !pinFwd;
            const bool isMovingForward = pinFwd && !pinBack;

            onPositionPulse(interruptTime, isMovingForward, isMovingBack);
        }
    }
}


auto ResistanceLevel::level() const -> uint8_t {
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
    // We need to cast 'this' to non-const to access sys?
    // Or make sys mutable.
    // Or just accept that sys.noInterrupts() changes system state, not object state.
    const ResistanceLevel *nonConstThis = const_cast<ResistanceLevel *>(this);

    nonConstThis->sys.disableInterrupts();
    const uint16_t snap = positionChangeCounter;
    nonConstThis->sys.enableInterrupts();
    return snap;
}

void ResistanceLevel::updateLevelFromCounter() {
    const auto *const levelRange = std::find_if(LEVEL_RANGES.cbegin(), LEVEL_RANGES.cend(),
                                 [this](const LevelRange &range) -> bool {
                                     return positionChangeCounter >= range.minCount && positionChangeCounter <= range.maxCount;
                                 });

    const bool isLevelFound = levelRange != LEVEL_RANGES.cend();
    if (isLevelFound) {
        currentLevel = levelRange->level;
    }
}

void ResistanceLevel::onLimitReset() {
    positionChangeCounter = 0;
    currentLevel = MIN_LEVEL;
}

void ResistanceLevel::onPositionPulse(const uint32_t timestampMs, const bool isMovingForward, const bool isMovingBack) {
    const unsigned long sampleTime = timestampMs - lastPulseTimestamp;

    // 1. Debounce Logic
    if (sampleTime > DEBOUNCE_TIME_MS) {
        lastPulseTimestamp = timestampMs;

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

void ResistanceLevel::poll() {
    // Was update()
    // 1. Read Inputs
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
        onPositionPulse(sys.millis(), isMovingForward, isMovingBack);
    }
}
