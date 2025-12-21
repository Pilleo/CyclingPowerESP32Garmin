#include "CadenceLogic.h"

auto CadenceLogic::getGattLastCrankRevolutionTimestamp() const -> uint16_t {
    return _gattLastCrankRevolutionTimestamp;
}

auto CadenceLogic::totalRevs() const -> uint32_t {
    return _totalRev;
}

auto CadenceLogic::lastTimestamp() const -> uint32_t {
    return _elapsedTimestamp;
}

CadenceLogic::CadenceLogic(ISensorDriver& driver, ISystemWrapper& sys) noexcept
    : _driver(driver),
      _sys(sys),
      _rpm(0.0F),
      _lastIntervalTime(0),
      _lastKnownEventCount(0),
      _elapsedTimestamp(0),
      _elapsedSampleTimeStamp(0),
      _rev(0),
      _totalRev(0),
      _gattLastCrankRevolutionTimestamp(0) {
}

void CadenceLogic::begin() {
    _driver.begin();
    _lastKnownEventCount = _driver.getEventCount();
}

namespace {
    auto calculateRpmFromRevolutions(const uint8_t revolutions, const uint32_t revolutionsTime) -> float {
        if (revolutionsTime == 0) {
            return 0.0F;
        }
        const float instantaneousRpm = 60000.0F / (static_cast<float>(revolutionsTime) / static_cast<float>(revolutions));
        return instantaneousRpm;
    }

    constexpr float MILLISECONDS_IN_SECOND = 1000.0F;
    constexpr float GATT_TIME_UNITS_PER_SECOND = 1024.0F;
} // namespace

void CadenceLogic::onPulse(uint32_t now) {
    if (now - _elapsedSampleTimeStamp > MIN_DELAY_BETWEEN_FULL_ROTATION_MS) {
        _rev++;
        _totalRev++;
        _elapsedSampleTimeStamp = now;
    }
}

void CadenceLogic::update() {
    _driver.update();
    uint32_t currentEventCount = _driver.getEventCount();
    if (currentEventCount != _lastKnownEventCount) {
        _lastKnownEventCount = currentEventCount;
        onPulse(_driver.getLastEventTime());
    }
}

bool CadenceLogic::shouldSleep() const {
    return (_sys.millis() - _driver.getLastEventTime()) > DEEP_SLEEP_TIMEOUT_MS;
}

auto CadenceLogic::cadence() -> float {
    const uint32_t mls = _sys.millis();

    const uint8_t snapshotRev = _rev;
    const uint32_t snapshotTimestamp = _elapsedTimestamp;

    const uint32_t intervalTime = mls - snapshotTimestamp;

    if (intervalTime > MIN_DELAY_BETWEEN_FULL_ROTATION_MS) {
        if (snapshotRev == 0) {
            if (intervalTime > MAX_IDLE_TIMEOUT_MS) {
                _rpm = 0.0F;
            } else if (_rpm > 0 && _lastIntervalTime > 0 && intervalTime > _lastIntervalTime) {
                _rpm = calculateRpmFromRevolutions(1, intervalTime);
            }
        } else {
            _rpm = calculateRpmFromRevolutions(snapshotRev, intervalTime);

            _rev = 0;
            _elapsedTimestamp = mls;

            _lastIntervalTime = intervalTime;
            _gattLastCrankRevolutionTimestamp = (_gattLastCrankRevolutionTimestamp +
                                                 static_cast<uint16_t>((static_cast<float>(intervalTime) / MILLISECONDS_IN_SECOND) * GATT_TIME_UNITS_PER_SECOND));
        }
    }

    return _rpm;
}
