#include <unity.h>

#include "CpsControlPoint.h"

void test_set_cumulative_value_offsets_only_wheel_revolutions() {
  CpsControlPoint controlPoint;
  const uint8_t request[] = {0x01, 0x39, 0x30, 0x00, 0x00};

  const CpsControlPointResponse response =
      controlPoint.write(request, sizeof(request), 30);

  TEST_ASSERT_EQUAL_UINT8(0x20, response.bytes[0]);
  TEST_ASSERT_EQUAL_UINT8(0x01, response.bytes[1]);
  TEST_ASSERT_EQUAL_UINT8(0x01, response.bytes[2]);
  TEST_ASSERT_EQUAL_UINT32(12345, controlPoint.apply(30));
  TEST_ASSERT_EQUAL_UINT32(12348, controlPoint.apply(33));
}

void test_rejects_malformed_and_unsupported_requests() {
  CpsControlPoint controlPoint;
  const uint8_t malformed[] = {0x01, 0x01};
  const uint8_t unsupported[] = {0x03};

  const CpsControlPointResponse malformedResponse =
      controlPoint.write(malformed, sizeof(malformed), 10);
  const CpsControlPointResponse unsupportedResponse =
      controlPoint.write(unsupported, sizeof(unsupported), 10);

  TEST_ASSERT_EQUAL_UINT8(0x03, malformedResponse.bytes[2]);
  TEST_ASSERT_EQUAL_UINT8(0x02, unsupportedResponse.bytes[2]);
  TEST_ASSERT_EQUAL_UINT32(10, controlPoint.apply(10));
}

void test_rejected_requests_preserve_an_established_wheel_offset() {
  CpsControlPoint controlPoint;
  const uint8_t accepted[] = {0x01, 0x64, 0x00, 0x00, 0x00};
  const uint8_t malformed[] = {0x01, 0x01};
  const uint8_t unsupported[] = {0x03};

  controlPoint.write(accepted, sizeof(accepted), 30);
  const CpsControlPointResponse malformedResponse =
      controlPoint.write(malformed, sizeof(malformed), 33);
  const CpsControlPointResponse unsupportedResponse =
      controlPoint.write(unsupported, sizeof(unsupported), 33);

  TEST_ASSERT_EQUAL_UINT8(0x03, malformedResponse.bytes[2]);
  TEST_ASSERT_EQUAL_UINT8(0x02, unsupportedResponse.bytes[2]);
  TEST_ASSERT_EQUAL_UINT32(103, controlPoint.apply(33));
}

void test_clamps_adjusted_wheel_count_to_uint32_range() {
  CpsControlPoint controlPoint;
  const uint8_t zeroRequest[] = {0x01, 0x00, 0x00, 0x00, 0x00};
  controlPoint.write(zeroRequest, sizeof(zeroRequest), 30);
  TEST_ASSERT_EQUAL_UINT32(0, controlPoint.apply(0));

  CpsControlPoint upperControlPoint;
  const uint8_t maximumRequest[] = {0x01, 0xFF, 0xFF, 0xFF, 0xFF};
  upperControlPoint.write(maximumRequest, sizeof(maximumRequest), 0);
  TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, upperControlPoint.apply(3));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_set_cumulative_value_offsets_only_wheel_revolutions);
  RUN_TEST(test_rejects_malformed_and_unsupported_requests);
  RUN_TEST(test_rejected_requests_preserve_an_established_wheel_offset);
  RUN_TEST(test_clamps_adjusted_wheel_count_to_uint32_range);
  return UNITY_END();
}
