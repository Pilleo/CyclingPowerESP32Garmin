#ifndef CADENCE_MODEL_H
#define CADENCE_MODEL_H

#include <cstdint>

enum class CadencePhase : uint8_t {
  Starting,
  Moving,
  Stopped,
  SleepReady,
};

struct CadenceReading {
  float rpm = 0.0F;
  uint32_t totalCrankRevolutions = 0;
  uint16_t lastCrankEventTime1024 = 0;
  CadencePhase phase = CadencePhase::Starting;

  constexpr CadenceReading() = default;
  constexpr CadenceReading(const float rpmValue,
                           const uint32_t totalRevolutionsValue,
                           const uint16_t eventTimeValue,
                           const CadencePhase phaseValue)
      : rpm(rpmValue),
        totalCrankRevolutions(totalRevolutionsValue),
        lastCrankEventTime1024(eventTimeValue),
        phase(phaseValue) {}
};

struct CadenceModelState {
  float rpm = 0.0F;
  uint32_t totalCrankRevolutions = 0;
  uint32_t acceptedPulsesSinceEvaluation = 0;
  uint32_t lastAcceptedPulseTimeMs = 0;
  uint32_t lastEvaluationTimeMs = 0;
  uint32_t lastEvaluationIntervalMs = 0;
  uint16_t lastCrankEventTime1024 = 0;
  CadencePhase phase = CadencePhase::Starting;
};

struct CadenceEvaluation {
  CadenceModelState nextState;
  CadenceReading reading;

  constexpr CadenceEvaluation(const CadenceModelState& nextStateValue,
                              const CadenceReading& readingValue)
      : nextState(nextStateValue), reading(readingValue) {}
};

constexpr uint32_t CADENCE_MIN_REVOLUTION_INTERVAL_MS = 330;
constexpr uint32_t CADENCE_STOP_TIMEOUT_MS = 5000;
constexpr uint32_t CADENCE_SLEEP_TIMEOUT_MS = 15U * 60U * 1000U;

constexpr uint16_t cadenceTicksFromMilliseconds(const uint32_t milliseconds) {
  return static_cast<uint16_t>((static_cast<uint64_t>(milliseconds) * 1024U) /
                               1000U);
}

inline auto recordCadencePulse(CadenceModelState state,
                               const uint32_t pulseTimeMs)
    -> CadenceModelState {
  const uint32_t pulseInterval = pulseTimeMs - state.lastAcceptedPulseTimeMs;
  if (pulseInterval <= CADENCE_MIN_REVOLUTION_INTERVAL_MS) {
    return state;
  }

  state.totalCrankRevolutions++;
  state.acceptedPulsesSinceEvaluation++;
  state.lastAcceptedPulseTimeMs = pulseTimeMs;
  state.lastCrankEventTime1024 = static_cast<uint16_t>(
      state.lastCrankEventTime1024 + cadenceTicksFromMilliseconds(pulseInterval));
  return state;
}

inline auto evaluateCadence(CadenceModelState state, const uint32_t nowMs)
    -> CadenceEvaluation {
  const uint32_t evaluationInterval = nowMs - state.lastEvaluationTimeMs;

  if (evaluationInterval > CADENCE_MIN_REVOLUTION_INTERVAL_MS) {
    if (state.acceptedPulsesSinceEvaluation > 0U) {
      state.rpm =
          (60000.0F * static_cast<float>(state.acceptedPulsesSinceEvaluation)) /
          static_cast<float>(evaluationInterval);
      state.acceptedPulsesSinceEvaluation = 0;
      state.lastEvaluationTimeMs = nowMs;
      state.lastEvaluationIntervalMs = evaluationInterval;
      state.phase = CadencePhase::Moving;
    } else {
      const uint32_t idleTime = nowMs - state.lastAcceptedPulseTimeMs;
      if (idleTime > CADENCE_SLEEP_TIMEOUT_MS) {
        state.rpm = 0.0F;
        state.phase = CadencePhase::SleepReady;
      } else if (idleTime > CADENCE_STOP_TIMEOUT_MS) {
        state.rpm = 0.0F;
        state.phase = CadencePhase::Stopped;
      } else if (state.rpm > 0.0F && state.lastEvaluationIntervalMs > 0U &&
                 evaluationInterval > state.lastEvaluationIntervalMs) {
        state.rpm = 60000.0F / static_cast<float>(evaluationInterval);
      }
    }
  }

  return CadenceEvaluation(
      state, CadenceReading(state.rpm, state.totalCrankRevolutions,
                            state.lastCrankEventTime1024, state.phase));
}

#endif // CADENCE_MODEL_H
