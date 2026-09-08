#include <unity.h>
#include "CadenceModel.h"

void test_pulse_timestamps_are_independent_of_evaluation_delay() {
  CadenceModelState state{};

  state = recordCadencePulse(state, 1000);
  state = recordCadencePulse(state, 2000);
  const CadenceEvaluation evaluation = evaluateCadence(state, 3000);

  TEST_ASSERT_EQUAL_UINT32(2, evaluation.reading.totalCrankRevolutions);
  TEST_ASSERT_EQUAL_UINT16(2048, evaluation.reading.lastCrankEventTime1024);
  TEST_ASSERT_FLOAT_WITHIN(0.01F, 40.0F, evaluation.reading.rpm);
  TEST_ASSERT_EQUAL(CadencePhase::Moving, evaluation.reading.phase);
}

void test_model_transitions_from_stopped_to_sleep_ready() {
  CadenceModelState state{};
  state = recordCadencePulse(state, 1000);
  state = evaluateCadence(state, 1000).nextState;

  const CadenceEvaluation stopped = evaluateCadence(state, 6001);
  TEST_ASSERT_EQUAL(CadencePhase::Stopped, stopped.reading.phase);
  TEST_ASSERT_EQUAL_FLOAT(0.0F, stopped.reading.rpm);

  const CadenceEvaluation sleeping = evaluateCadence(stopped.nextState, 901001);
  TEST_ASSERT_EQUAL(CadencePhase::SleepReady, sleeping.reading.phase);
}

void test_model_accepts_a_pulse_after_millis_rollover() {
  CadenceModelState state{};
  state = recordCadencePulse(state, 0xFFFFFE00U);
  const uint16_t firstEventTime = state.lastCrankEventTime1024;
  state = recordCadencePulse(state, 0x000001E8U);

  TEST_ASSERT_EQUAL_UINT32(2, state.totalCrankRevolutions);
  TEST_ASSERT_EQUAL_UINT16(1024,
                           static_cast<uint16_t>(
                               state.lastCrankEventTime1024 - firstEventTime));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_pulse_timestamps_are_independent_of_evaluation_delay);
  RUN_TEST(test_model_transitions_from_stopped_to_sleep_ready);
  RUN_TEST(test_model_accepts_a_pulse_after_millis_rollover);
  return UNITY_END();
}
