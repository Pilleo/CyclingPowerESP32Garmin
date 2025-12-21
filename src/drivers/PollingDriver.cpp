#include "PollingDriver.h"

PollingDriver::PollingDriver(ISystemWrapper& sys, uint8_t pin)
    : _sys(sys),
      _pin(pin),
      _lastState(false),
      _eventCount(0),
      _lastEventTime(0) {}

void PollingDriver::begin() {
    _lastState = (_sys.digitalRead(_pin) != 0);
}

void PollingDriver::update() {
    bool currentState = (_sys.digitalRead(_pin) != 0);
    if (_lastState != currentState) {
        _lastState = currentState;
        // Detect falling edge, assuming active-low sensor
        if (!currentState) {
            _eventCount++;
            _lastEventTime = _sys.millis();
        }
    }
}

uint32_t PollingDriver::getEventCount() const {
    return _eventCount;
}

uint32_t PollingDriver::getLastEventTime() const {
    return _lastEventTime;
}
