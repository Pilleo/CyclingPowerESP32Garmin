#include <unity.h>
#include "ResistanceLevel.h"
#include "../common/MockSystemWrapper.h"

namespace {
constexpr ResistanceLevelPins PINS = {
    .backwards = 4, .limit = 18, .position = 23, .forwards = 19};
}

void test_direction_classification_distinguishes_all_pin_states() {
  TEST_ASSERT_EQUAL(ResistanceDirection::Stopped,
                    classifyResistanceDirection(false, false));
  TEST_ASSERT_EQUAL(ResistanceDirection::Forward,
                    classifyResistanceDirection(true, false));
  TEST_ASSERT_EQUAL(ResistanceDirection::Backward,
                    classifyResistanceDirection(false, true));
  TEST_ASSERT_EQUAL(ResistanceDirection::Invalid,
                    classifyResistanceDirection(true, true));
}

void test_completed_run_reports_polling_and_direction_evidence_once() {
  MockSystemWrapper sys;
  sys.setMillis(2000);
  sys.setPinState(PINS.limit, false);
  sys.setPinState(PINS.backwards, false);
  sys.setPinState(PINS.forwards, false);
  sys.setPinState(PINS.position, false);
  ResistanceLevel resistance(PINS, sys);
  resistance.begin();

  sys.setPinState(PINS.forwards, true);
  resistance.poll();

  sys.advanceTime(10);
  sys.setPinState(PINS.position, true);
  resistance.poll();

  sys.advanceTime(10);
  sys.setPinState(PINS.backwards, true);
  resistance.poll();

  sys.advanceTime(10);
  sys.setPinState(PINS.forwards, false);
  sys.setPinState(PINS.backwards, false);
  resistance.poll();

  sys.advanceTime(10);
  sys.setPinState(PINS.forwards, true);
  resistance.poll();

  sys.advanceTime(10);
  sys.setPinState(PINS.position, false);
  resistance.poll();

  sys.advanceTime(10);
  sys.setPinState(PINS.forwards, false);
  resistance.poll();
  sys.advanceTime(100);
  resistance.poll();

  ResistanceRunDiagnostics report{};
  TEST_ASSERT_TRUE(resistance.takeCompletedRunDiagnostics(report));
  TEST_ASSERT_EQUAL(ResistanceDirection::Forward, report.direction);
  TEST_ASSERT_EQUAL_UINT16(0, report.startCounter);
  TEST_ASSERT_EQUAL_UINT16(2, report.endCounter);
  TEST_ASSERT_EQUAL_UINT32(2, report.observedEdges);
  TEST_ASSERT_EQUAL_UINT32(1, report.invalidDirectionSamples);
  TEST_ASSERT_EQUAL_UINT32(1, report.stoppedSamplesDuringRun);
  TEST_ASSERT_EQUAL_UINT32(10, report.maxPollGapMs);
  TEST_ASSERT_EQUAL_UINT32(40, report.minimumAcceptedEdgeIntervalMs);
  TEST_ASSERT_FALSE(resistance.takeCompletedRunDiagnostics(report));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_direction_classification_distinguishes_all_pin_states);
  RUN_TEST(test_completed_run_reports_polling_and_direction_evidence_once);
  return UNITY_END();
}
