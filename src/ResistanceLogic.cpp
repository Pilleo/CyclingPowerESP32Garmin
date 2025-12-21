#include "ResistanceLogic.h"
#include <algorithm>
#include <array>
#include <cstdio>

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

ResistanceLogic::ResistanceLogic(const ResistanceLogicPins& pins, ISensorDriver& positionDriver, ISystemWrapper& sys) noexcept
    : _positionDriver(positionDriver),
      _sys(sys),
      _limitPin(pins.limit),
      _backwardsPin(pins.backwards),
      _forwardsPin(pins.forwards),
      _lastKnownEventCount(0),
      _currentLevel(MIN_LEVEL),
      _lastPulseTimestamp(0),
      _prevDirectionWasForward(true),
      _positionChangeCounter(0) {
}

void ResistanceLogic::begin() {
    _positionDriver.begin();
    _lastKnownEventCount = _positionDriver.getEventCount();
}

auto ResistanceLogic::level() const -> uint8_t {
    return _currentLevel;
}

auto ResistanceLogic::getPositionChangeCounter() const -> uint16_t {
    return _positionChangeCounter;
}

void ResistanceLogic::updateLevelFromCounter() {
    const auto* const levelRange = std::find_if(LEVEL_RANGES.cbegin(), LEVEL_RANGES.cend(),
                                 [this](const LevelRange& range) -> bool {
                                     return _positionChangeCounter >= range.minCount && _positionChangeCounter <= range.maxCount;
                                 });

    if (levelRange != LEVEL_RANGES.cend()) {
        _currentLevel = levelRange->level;
    }
}

void ResistanceLogic::onLimitReset() {
    _positionChangeCounter = 0;
    _currentLevel = MIN_LEVEL;
}

void ResistanceLogic::handlePositionPulse(const uint32_t timestampMs, const bool isMovingForward, const bool isMovingBack) {
    const unsigned long sampleTime = timestampMs - _lastPulseTimestamp;

    if (sampleTime > DEBOUNCE_TIME_MS) {
        _lastPulseTimestamp = timestampMs;

        const bool isConsistent = (sampleTime < MOVEMENT_TIMEOUT_MS);
        const bool isLongPause = (sampleTime > PAUSE_TIMEOUT_MS);

        printf("ResistanceLogic: sampleTime=%lu, isConsistent=%d, isLongPause=%d, counter=%u\n", sampleTime, isConsistent, isLongPause, _positionChangeCounter);

        if (isMovingForward) {
            if ((isConsistent && _prevDirectionWasForward) || isLongPause) {
                _positionChangeCounter++;
            }
        } else if (_positionChangeCounter > 0 && isMovingBack) {
            if (isLongPause || (isConsistent && !_prevDirectionWasForward)) {
                _positionChangeCounter--;
            }
        }

        _prevDirectionWasForward = isMovingForward;
        updateLevelFromCounter();
    }
}

void ResistanceLogic::update() {
    _positionDriver.update();

    const bool pinBackRaw = (_sys.digitalRead(_backwardsPin) != 0);
    const bool pinForwardRaw = (_sys.digitalRead(_forwardsPin) != 0);
    const bool limitActive = (_sys.digitalRead(_limitPin) != 0);

    const bool isMovingBack = pinBackRaw && !pinForwardRaw;
    const bool isMovingForward = pinForwardRaw && !pinBackRaw;

    if (!isMovingForward && limitActive) {
        onLimitReset();
    }

    uint32_t currentEventCount = _positionDriver.getEventCount();
    if (currentEventCount != _lastKnownEventCount) {
        printf("ResistanceLogic: event received, old_count=%u, new_count=%u\n", _lastKnownEventCount, currentEventCount);
        _lastKnownEventCount = currentEventCount;
        handlePositionPulse(_positionDriver.getLastEventTime(), isMovingForward, isMovingBack);
    }
}
