#include "ResistanceLevel.h"

ResistanceLevel *ResistanceLevel::_instance = nullptr;

auto classifyResistanceDirection(const bool forwardActive,
                                 const bool backwardActive)
    -> ResistanceDirection {
  if (forwardActive == backwardActive) {
    return forwardActive ? ResistanceDirection::Invalid
                         : ResistanceDirection::Stopped;
  }
  return forwardActive ? ResistanceDirection::Forward
                       : ResistanceDirection::Backward;
}

ResistanceLevel::ResistanceLevel(const ResistanceLevelPins &pins,
                                 ISystemWrapper &sys) noexcept
    : sys(sys), limitPin(pins.limit), backwardsPin(pins.backwards),
      positionPin(pins.position), forwardsPin(pins.forwards),
      oldPositionState(false), currentLevel(MIN_LEVEL), lastPulseTimestamp(0),
      prevDirectionWasForward(true), positionChangeCounter(0) {}

void ResistanceLevel::begin() {
  oldPositionState = (sys.digitalRead(positionPin) != 0);
  _lastPollTimeMs = sys.millis();
  _hasPollTime = true;
}

auto ResistanceLevel::takeCompletedRunDiagnostics(
    ResistanceRunDiagnostics &diagnostics) -> bool {
  if (!_completedRunAvailable) {
    return false;
  }
  diagnostics = _completedRun;
  _completedRunAvailable = false;
  return true;
}

void ResistanceLevel::beginDiagnosticRun(
    const ResistanceDirection direction) {
  _activeRun = {};
  _activeRun.direction = direction;
  _activeRun.startCounter = positionChangeCounter;
  _runActive = true;
  _stopping = false;
  _pendingStoppedSamples = 0;
  _hasObservedEdgeTime = false;
}

void ResistanceLevel::completeDiagnosticRun() {
  _activeRun.endCounter = positionChangeCounter;
  _completedRun = _activeRun;
  _completedRunAvailable = true;
  _runActive = false;
  _stopping = false;
  _pendingStoppedSamples = 0;
  _hasObservedEdgeTime = false;
}

void ResistanceLevel::updateRunDiagnostics(
    const ResistanceDirection direction, const uint32_t nowMs,
    const uint32_t pollGapMs) {
  const bool moving = direction == ResistanceDirection::Forward ||
                      direction == ResistanceDirection::Backward;
  if (!_runActive && moving) {
    beginDiagnosticRun(direction);
  }
  if (!_runActive) {
    return;
  }

  if (direction == ResistanceDirection::Invalid) {
    ++_activeRun.invalidDirectionSamples;
    return;
  }
  if (direction == ResistanceDirection::Stopped) {
    if (!_stopping) {
      _stopping = true;
      _stopStartedTimeMs = nowMs;
      _pendingStoppedSamples = 1;
    } else if (nowMs - _stopStartedTimeMs >= RUN_STOP_CONFIRMATION_MS) {
      completeDiagnosticRun();
    } else {
      ++_pendingStoppedSamples;
    }
    return;
  }

  if (_stopping) {
    _activeRun.stoppedSamplesDuringRun += _pendingStoppedSamples;
    _stopping = false;
    _pendingStoppedSamples = 0;
  }
  if (pollGapMs > _activeRun.maxPollGapMs) {
    _activeRun.maxPollGapMs = pollGapMs;
  }
}

void ResistanceLevel::recordObservedEdge(const uint32_t nowMs) {
  if (!_runActive) {
    return;
  }
  ++_activeRun.observedEdges;
  if (_hasObservedEdgeTime) {
    const uint32_t interval = nowMs - _lastObservedEdgeTimeMs;
    if (_activeRun.minimumAcceptedEdgeIntervalMs == 0 ||
        interval < _activeRun.minimumAcceptedEdgeIntervalMs) {
      _activeRun.minimumAcceptedEdgeIntervalMs = interval;
    }
  }
  _lastObservedEdgeTimeMs = nowMs;
  _hasObservedEdgeTime = true;
}

void ResistanceLevel::enableInterrupt() { _instance = this; }

void ResistanceLevel::isrPosition() {
  if (_instance != nullptr) {
    _instance->handlePositionInterrupt();
  }
}

void ISR_ATTR ResistanceLevel::handlePositionInterrupt() {
  const bool currentPos = (sys.digitalRead(positionPin) != 0);
  if (currentPos != oldPositionState) {
    oldPositionState = currentPos;

    const uint32_t now = sys.millis();
    const bool pinBack = (sys.digitalRead(backwardsPin) != 0);
    const bool pinFwd = (sys.digitalRead(forwardsPin) != 0);
    const bool isMovingBack = pinBack && !pinFwd;
    const bool isMovingForward = pinFwd && !pinBack;

    // Safety: Check limit switch during position pulses too (catch-all for
    // drift)
    if (!isMovingForward && sys.digitalRead(limitPin) != 0) {
      onLimitReset();
    }

    onPositionPulse(now, isMovingForward, isMovingBack);
  }
}

void ResistanceLevel::isrLimit() {
  if (_instance != nullptr) {
    _instance->handleLimitInterrupt();
  }
}

void ISR_ATTR ResistanceLevel::handleLimitInterrupt() {
  const bool pinFwd = (sys.digitalRead(forwardsPin) != 0);
  const bool pinBack = (sys.digitalRead(backwardsPin) != 0);
  const bool isMovingForward = pinFwd && !pinBack;

  if (!isMovingForward && sys.digitalRead(limitPin) != 0) {
    onLimitReset();
  }
}

void ResistanceLevel::update() {
  // In interrupt mode, this function is now a no-op.
  // The logic has been moved to the ISR handlers.
}

auto ResistanceLevel::level() const -> uint8_t { return getLevel(); }

auto ResistanceLevel::getLevel() const -> uint8_t {
  // uint8_t is usually atomic, but let's be strict
  // Since this is 'const', we need to cast away constness or rely on mutable
  // sys? ISystemWrapper& sys is not const in the header.

  // Actually, we can just return the volatile.
  // The risk is low for level.
  return currentLevel;
}

auto ResistanceLevel::getPositionChangeCounter() const -> uint16_t {
  // We need to cast 'this' to non-const to access sys?
  // Or make sys mutable.
  // Or just accept that sys.noInterrupts() changes system state, not object
  // state.
  const ResistanceLevel *nonConstThis = const_cast<ResistanceLevel *>(this);

  nonConstThis->sys.disableInterrupts();
  const uint16_t snap = positionChangeCounter;
  nonConstThis->sys.enableInterrupts();
  return snap;
}

void ISR_ATTR ResistanceLevel::onLimitReset() {
  _model = reduceResistance(
      _model, {lastPulseTimestamp, ResistanceDirection::Stopped, false, true});
  positionChangeCounter = _model.counter;
  currentLevel = _model.level;
}

void ISR_ATTR ResistanceLevel::onPositionPulse(const uint32_t timestampMs,
                                               const bool isMovingForward,
                                               const bool isMovingBack) {
  const ResistanceDirection direction =
      classifyResistanceDirection(isMovingForward, isMovingBack);
  _model.counter = positionChangeCounter;
  _model.level = currentLevel;
  _model.lastAcceptedEdgeTimeMs = lastPulseTimestamp;
  _model.lastValidDirection = prevDirectionWasForward
                                  ? ResistanceDirection::Forward
                                  : ResistanceDirection::Backward;
  _model = reduceResistance(_model, {timestampMs, direction, true, false});
  positionChangeCounter = _model.counter;
  currentLevel = _model.level;
  lastPulseTimestamp = _model.lastAcceptedEdgeTimeMs;
  prevDirectionWasForward =
      _model.lastValidDirection == ResistanceDirection::Forward;
}

void ResistanceLevel::poll() {
  // Was update()
  // 1. Read Inputs
  const bool pinBackRaw = (sys.digitalRead(backwardsPin) != 0);
  const bool pinForwardRaw = (sys.digitalRead(forwardsPin) != 0);
  const bool pinPosition = (sys.digitalRead(positionPin) != 0);
  const bool limitActive = (sys.digitalRead(limitPin) != 0);

  const uint32_t now = sys.millis();
  const uint32_t pollGapMs = _hasPollTime ? now - _lastPollTimeMs : 0;
  _lastPollTimeMs = now;
  _hasPollTime = true;

  // Derived Direction States
  const bool isMovingBack = pinBackRaw && !pinForwardRaw;
  const bool isMovingForward = pinForwardRaw && !pinBackRaw;
  const ResistanceDirection direction =
      classifyResistanceDirection(pinForwardRaw, pinBackRaw);
  updateRunDiagnostics(direction, now, pollGapMs);

  // 2. Handle Limit Reset
  if (!isMovingForward && limitActive) {
    onLimitReset();
  }

  // 3. Detect Edge
  if (pinPosition != oldPositionState) {
    oldPositionState = pinPosition;
    const bool acceptedEdge = now - lastPulseTimestamp > DEBOUNCE_TIME_MS;
    onPositionPulse(now, isMovingForward, isMovingBack);
    if (acceptedEdge) {
      recordObservedEdge(now);
    }
  }
}
