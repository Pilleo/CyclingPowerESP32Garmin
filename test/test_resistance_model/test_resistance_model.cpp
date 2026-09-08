#include <unity.h>

#include "ResistanceModel.h"

#include <cstdint>

void test_invalid_edge_does_not_corrupt_last_valid_direction() {
  ResistanceModelState state{};
  state.lastAcceptedEdgeTimeMs = 2000;
  state.lastValidDirection = ResistanceDirection::Forward;

  state = reduceResistance(
      state, {2020, ResistanceDirection::Forward, true, false});
  state = reduceResistance(
      state, {2040, ResistanceDirection::Invalid, true, false});
  state = reduceResistance(
      state, {2060, ResistanceDirection::Forward, true, false});

  TEST_ASSERT_EQUAL_UINT16(2, state.counter);
  TEST_ASSERT_EQUAL(ResistanceDirection::Forward, state.lastValidDirection);
}

void test_stopped_edge_does_not_corrupt_last_valid_direction() {
  ResistanceModelState state{};
  state.lastAcceptedEdgeTimeMs = 2000;
  state.lastValidDirection = ResistanceDirection::Backward;
  state.counter = 36;

  state = reduceResistance(
      state, {2020, ResistanceDirection::Backward, true, false});
  state = reduceResistance(
      state, {2040, ResistanceDirection::Stopped, true, false});
  state = reduceResistance(
      state, {2060, ResistanceDirection::Backward, true, false});

  TEST_ASSERT_EQUAL_UINT16(34, state.counter);
  TEST_ASSERT_EQUAL(ResistanceDirection::Backward, state.lastValidDirection);
}

void test_level_boundaries_and_gaps_preserve_existing_mapping() {
  using resistance_model_detail::levelForCounter;
  TEST_ASSERT_EQUAL_UINT8(1, levelForCounter(29, 9));
  TEST_ASSERT_EQUAL_UINT8(9, levelForCounter(30, 9));
  TEST_ASSERT_EQUAL_UINT8(9, levelForCounter(169, 9));
  TEST_ASSERT_EQUAL_UINT8(2, levelForCounter(170, 9));
  TEST_ASSERT_EQUAL_UINT8(2, levelForCounter(190, 9));
  TEST_ASSERT_EQUAL_UINT8(9, levelForCounter(191, 9));
  TEST_ASSERT_EQUAL_UINT8(15, levelForCounter(681, 9));
  TEST_ASSERT_EQUAL_UINT8(15, levelForCounter(690, 9));
  TEST_ASSERT_EQUAL_UINT8(9, levelForCounter(691, 9));
  TEST_ASSERT_EQUAL_UINT8(16, levelForCounter(692, 9));
  TEST_ASSERT_EQUAL_UINT8(16, levelForCounter(UINT16_MAX, 9));
}

void test_timing_boundaries_and_rollover_are_unsigned() {
  ResistanceModelState state{};
  state.counter = 10;
  state.lastAcceptedEdgeTimeMs = 1000;
  state.lastValidDirection = ResistanceDirection::Forward;

  state = reduceResistance(
      state, {1008, ResistanceDirection::Forward, true, false});
  TEST_ASSERT_EQUAL_UINT16(10, state.counter);
  TEST_ASSERT_EQUAL_UINT32(1000, state.lastAcceptedEdgeTimeMs);

  state = reduceResistance(
      state, {1049, ResistanceDirection::Forward, true, false});
  TEST_ASSERT_EQUAL_UINT16(11, state.counter);

  state.lastAcceptedEdgeTimeMs = UINT32_MAX - 10U;
  state = reduceResistance(
      state, {10, ResistanceDirection::Forward, true, false});
  TEST_ASSERT_EQUAL_UINT16(12, state.counter);
}

void test_limit_reset_and_overflow_match_existing_semantics() {
  ResistanceModelState state{};
  state.counter = UINT16_MAX;
  state.level = 16;
  state.lastAcceptedEdgeTimeMs = 100;

  state = reduceResistance(
      state, {2000, ResistanceDirection::Forward, true, false});
  TEST_ASSERT_EQUAL_UINT16(0, state.counter);
  TEST_ASSERT_EQUAL_UINT8(1, state.level);

  state.counter = 500;
  state.level = 7;
  state = reduceResistance(
      state, {2001, ResistanceDirection::Stopped, false, true});
  TEST_ASSERT_EQUAL_UINT16(0, state.counter);
  TEST_ASSERT_EQUAL_UINT8(1, state.level);
}

void test_limit() {}
void tearDown() {}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_invalid_edge_does_not_corrupt_last_valid_direction);
  RUN_TEST(test_stopped_edge_does_not_corrupt_last_valid_direction);
  RUN_TEST(test_level_boundaries_and_gaps_preserve_existing_mapping);
  RUN_TEST(test_timing_boundaries_and_rollover_are_unsigned);
  RUN_TEST(test_limit_reset_and_overflow_match_existing_semantics);
  return UNITY_END();
}
