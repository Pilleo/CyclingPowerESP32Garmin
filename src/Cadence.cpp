#include "Cadence.h"

Cadence *Cadence::_instance = nullptr;

auto Cadence::getGattLastCrankRevolutionTimestamp() const -> uint16_t {
  return _state.lastCrankEventTime1024;
}

auto Cadence::totalRevs() const -> uint32_t { return _state.totalCrankRevolutions; }

auto Cadence::lastTimestamp() const -> uint32_t { return _state.lastEvaluationTimeMs; }

void Cadence::enableInterrupt() {
  _instance = this;
  // We prepare the bridge, but we DO NOT call sys.attachInterrupt() yet.
  // That is the final step in the future.
}

// Static ISR: The Bridge
void Cadence::isr() {
  if (_instance != nullptr) {
    _instance->handleInterrupt();
  }
}

// Instance ISR: The Worker
void Cadence::handleInterrupt() {
  // Get time immediately
  uint32_t const now = sys.millis();
  onPulse(now);
}

Cadence::Cadence(const uint8_t pinn, ISystemWrapper &sys) noexcept
    : sys(sys), pin(pinn), oldState(false), _state{} {}

void Cadence::begin() { oldState = (sys.digitalRead(pin) != 0); }

void Cadence::onPulse(uint32_t now) {
  _state = recordCadencePulse(_state, now);
}

void Cadence::poll() {
  // 1. Hardware Detection (Polling Driver)
  bool const currentState = sys.digitalRead(pin) != 0;
  if (oldState && !currentState) {
    onPulse(sys.millis());
  }
  oldState = currentState;
}

auto Cadence::cadence() -> float {
  return reading().rpm;
}

auto Cadence::reading() -> CadenceReading {
  const CadenceEvaluation evaluation = evaluateCadence(_state, sys.millis());
  _state = evaluation.nextState;
  return evaluation.reading;
}
