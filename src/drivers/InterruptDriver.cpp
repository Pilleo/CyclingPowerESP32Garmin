#include "InterruptDriver.h"

InterruptDriver* InterruptDriver::_instance = nullptr;

InterruptDriver::InterruptDriver(ISystemWrapper& sys, uint8_t pin)
    : _sys(sys),
      _pin(pin),
      _eventCount(0),
      _lastEventTime(0),
      _lastDebounceTime(0) {}

void InterruptDriver::begin() {
    _instance = this;
    _sys.attachInterrupt(_pin, isr, FALLING);
}

void InterruptDriver::update() {
    // Not needed for interrupt-driven driver
}

uint32_t InterruptDriver::getEventCount() const {
    uint32_t count;
    _sys.disableInterrupts();
    count = _eventCount;
    _sys.enableInterrupts();
    return count;
}

uint32_t InterruptDriver::getLastEventTime() const {
    uint32_t time;
    _sys.disableInterrupts();
    time = _lastEventTime;
    _sys.enableInterrupts();
    return time;
}

void InterruptDriver::handleInterrupt() {
    uint32_t now = _sys.millis();
    if (now - _lastDebounceTime > DEBOUNCE_TIME_MS) {
        _lastDebounceTime = now;
        _eventCount++;
        _lastEventTime = now;
    }
}

void ISR_ATTR InterruptDriver::isr() {
    if (_instance) {
        _instance->handleInterrupt();
    }
}
