#include "../../src/CscPacketGenerator.h"
#include <cstring>
#include <unity.h>

namespace {
constexpr float WHEEL_CIRCUMFERENCE_METERS = 2.1F;

uint32_t readUint32Le(const uint8_t *bytes) {
  uint32_t value;
  std::memcpy(&value, bytes, sizeof(value));
  return value;
}

uint16_t readUint16Le(const uint8_t *bytes) {
  uint16_t value;
  std::memcpy(&value, bytes, sizeof(value));
  return value;
}

float calculateSpeedKmh(const uint8_t *first, const uint8_t *second) {
  const uint32_t wheelRevolutionDelta =
      readUint32Le(second + 1) - readUint32Le(first + 1);
  const uint16_t wheelEventTimeDelta = static_cast<uint16_t>(
      readUint16Le(second + 5) - readUint16Le(first + 5));
  return wheelRevolutionDelta * WHEEL_CIRCUMFERENCE_METERS * 1024.0F /
         wheelEventTimeDelta * 3.6F;
}
} // namespace

void setUp() {}
void tearDown() {}

void test_csc_packet_structure_compliance() {
  uint8_t buffer[CscPacketGenerator::MAX_PACKET_SIZE] = {};
  const uint8_t expected[] = {
      0x03,                    // Wheel and crank data present
      0x1E, 0x00, 0x00, 0x00, // 10 crank revolutions x 3
      0x00, 0x04,              // Wheel event time, 1/1024 second
      0x0A, 0x00,              // Crank revolutions
      0x00, 0x04               // Crank event time, 1/1024 second
  };

  const size_t size = CscPacketGenerator::generatePacket(10, 1024, buffer);

  TEST_ASSERT_EQUAL(sizeof(expected), size);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, buffer, sizeof(expected));
}

void test_csc_consecutive_packets_produce_expected_speed() {
  uint8_t first[CscPacketGenerator::MAX_PACKET_SIZE] = {};
  uint8_t second[CscPacketGenerator::MAX_PACKET_SIZE] = {};
  CscPacketGenerator::generatePacket(10, 1024, first);
  CscPacketGenerator::generatePacket(11, 2048, second);

  const uint32_t wheelRevolutionDelta =
      readUint32Le(second + 1) - readUint32Le(first + 1);
  const uint16_t wheelEventTimeDelta = static_cast<uint16_t>(
      readUint16Le(second + 5) - readUint16Le(first + 5));

  TEST_ASSERT_EQUAL_UINT32(3, wheelRevolutionDelta);
  TEST_ASSERT_EQUAL_UINT16(1024, wheelEventTimeDelta);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 22.68F,
                           calculateSpeedKmh(first, second));
}

void test_csc_wheel_event_time_rollover_preserves_speed() {
  uint8_t first[CscPacketGenerator::MAX_PACKET_SIZE] = {};
  uint8_t second[CscPacketGenerator::MAX_PACKET_SIZE] = {};
  CscPacketGenerator::generatePacket(10, 65000, first);
  CscPacketGenerator::generatePacket(11, 488, second);

  const uint16_t firstWheelTime = readUint16Le(first + 5);
  const uint16_t secondWheelTime = readUint16Le(second + 5);
  const uint16_t wheelEventTimeDelta =
      static_cast<uint16_t>(secondWheelTime - firstWheelTime);

  TEST_ASSERT_TRUE(secondWheelTime < firstWheelTime);
  TEST_ASSERT_EQUAL_UINT16(1024, wheelEventTimeDelta);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 22.68F,
                           calculateSpeedKmh(first, second));
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_csc_packet_structure_compliance);
  RUN_TEST(test_csc_consecutive_packets_produce_expected_speed);
  RUN_TEST(test_csc_wheel_event_time_rollover_preserves_speed);
  return UNITY_END();
}
