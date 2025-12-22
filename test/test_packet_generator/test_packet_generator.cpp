#include "../../src/BlePacketGenerator.h"
#include <unity.h>

void setUp() {
  // No setup needed for static class
}

void tearDown() {
  // No cleanup needed
}

void test_packet_structure_compliance() {
  // Inputs
  uint16_t power = 250;      // 0x00FA
  uint32_t crankRevs = 10;   // 0x0000000A
  uint16_t crankTime = 1024; // 0x0400 (1 second)

  uint8_t buffer[20];
  memset(buffer, 0, sizeof(buffer));

  // Execute
  size_t len =
      BlePacketGenerator::generatePacket(power, crankRevs, crankTime, buffer);

  // Expected Output Analysis:
  // 1. Flags (2 bytes):
  //    CPM_FLAG_WHEEL_REV_DATA_PRESENT (0x10) | CPM_FLAG_CRANK_REV_DATA_PRESENT
  //    (0x20) = 0x30 Little Endian: 0x30, 0x00

  // 2. Instantaneous Power (2 bytes): 250 = 0x00FA
  //    Little Endian: 0xFA, 0x00

  // 3. Cumulative Wheel Revs (4 bytes):
  //    Logic: crankRevs * 31 = 10 * 31 = 310 = 0x00000136
  //    Little Endian: 0x36, 0x01, 0x00, 0x00

  // 4. Last Wheel Event Time (2 bytes):
  //    Logic: crankTime * 2 = 1024 * 2 = 2048 = 0x0800
  //    Little Endian: 0x00, 0x08

  // 5. Cumulative Crank Revs (2 bytes): 10 = 0x000A
  //    Little Endian: 0x0A, 0x00

  // 6. Last Crank Event Time (2 bytes): 1024 = 0x0400
  //    Little Endian: 0x00, 0x04

  // Total Size: 2 + 2 + 4 + 2 + 2 + 2 = 14 bytes

  uint8_t expected[] = {
      0x30, 0x00,             // Flags
      0xFA, 0x00,             // Power
      0x36, 0x01, 0x00, 0x00, // Wheel Revs
      0x00, 0x08,             // Wheel Time
      0x0A, 0x00,             // Crank Revs
      0x00, 0x04              // Crank Time
  };

  TEST_ASSERT_EQUAL(14, len);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, buffer, 14);
}

void test_packet_overflow_handling() {
  // Test with max values to ensure no overflow issues in calculation logic
  // Though generatePacket uses standard types, checking wheel logic is good.

  uint16_t power = 1000;
  uint32_t crankRevs = 100000;
  uint16_t crankTime = 65535;

  uint8_t buffer[20];
  size_t len =
      BlePacketGenerator::generatePacket(power, crankRevs, crankTime, buffer);

  TEST_ASSERT_EQUAL(14, len);

  // Verify Wheel Revs: 100000 * 31 = 3,100,000 (Fits in uint32)
  // 3,100,000 = 0x002F4D60
  // Little Endian: 60 4D 2F 00

  // Offset 4 is Wheel Revs
  TEST_ASSERT_EQUAL_HEX8(0x60, buffer[4]);
  TEST_ASSERT_EQUAL_HEX8(0x4D, buffer[5]);
  TEST_ASSERT_EQUAL_HEX8(0x2F, buffer[6]);
  TEST_ASSERT_EQUAL_HEX8(0x00, buffer[7]);
}

void test_packet_zero_values() {
  // Test with all zeros to ensure stability
  uint16_t power = 0;
  uint32_t crankRevs = 0;
  uint16_t crankTime = 0;

  uint8_t buffer[20];
  size_t len =
      BlePacketGenerator::generatePacket(power, crankRevs, crankTime, buffer);

  TEST_ASSERT_EQUAL(14, len);

  // Power (bytes 2-3) should be 0
  TEST_ASSERT_EQUAL_HEX8(0x00, buffer[2]);
  TEST_ASSERT_EQUAL_HEX8(0x00, buffer[3]);

  // Wheel Revs (bytes 4-7) should be 0
  TEST_ASSERT_EQUAL_HEX8(0x00, buffer[4]);
  TEST_ASSERT_EQUAL_HEX8(0x00, buffer[5]);
  TEST_ASSERT_EQUAL_HEX8(0x00, buffer[6]);
  TEST_ASSERT_EQUAL_HEX8(0x00, buffer[7]);
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_packet_structure_compliance);
  RUN_TEST(test_packet_overflow_handling);
  RUN_TEST(test_packet_zero_values);
  return UNITY_END();
}
