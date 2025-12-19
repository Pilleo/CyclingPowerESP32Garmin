#include <unity.h>
#include "BlePacketGenerator.h"

void setUp(void) {}
void tearDown(void) {}

void test_ble_packet_structure() {
    uint8_t buffer[20];
    
    // Inputs
    uint16_t power = 250;           // 0x00FA
    uint32_t crankRevs = 10;        // 0x0000000A
    uint16_t crankTime = 1024;      // 0x0400 (1 second)

    // Expected Output Calculations
    // Flags: 0x10 | 0x20 = 0x30 (Crank & Wheel Present)
    // Wheel Revs: 10 * 31 = 310 (0x00000136)
    // Wheel Time: 1024 * 2 = 2048 (0x0800)

    size_t size = BlePacketGenerator::generatePacket(power, crankRevs, crankTime, buffer);

    TEST_ASSERT_EQUAL(14, size); // Struct size check

    // Byte 0-1: Flags (0x0030) - Little Endian
    TEST_ASSERT_EQUAL_HEX8(0x30, buffer[0]); 
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer[1]);

    // Byte 2-3: Power (250 = 0x00FA) - Little Endian -> FA 00
    TEST_ASSERT_EQUAL_HEX8(0xFA, buffer[2]);
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer[3]);

    // Byte 4-7: Wheel Revs (310 = 0x00000136) -> 36 01 00 00
    TEST_ASSERT_EQUAL_HEX8(0x36, buffer[4]);
    TEST_ASSERT_EQUAL_HEX8(0x01, buffer[5]);
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer[6]);
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer[7]);

    // Byte 8-9: Wheel Time (2048 = 0x0800) -> 00 08
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer[8]);
    TEST_ASSERT_EQUAL_HEX8(0x08, buffer[9]);

    // Byte 10-11: Crank Revs (10 = 0x000A) -> 0A 00
    TEST_ASSERT_EQUAL_HEX8(0x0A, buffer[10]);
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer[11]);

    // Byte 12-13: Crank Time (1024 = 0x0400) -> 00 04
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer[12]);
    TEST_ASSERT_EQUAL_HEX8(0x04, buffer[13]);
}

void test_ble_packet_overflow_handling() {
    uint8_t buffer[20];
    
    // Test UINT16 Time Overflow
    // Max UINT16 is 65535. Input 65530.
    // Logic: WheelTime = CrankTime * 2 = 131060.
    // 131060 % 65536 = 65524 (0xFFF4)
    
    BlePacketGenerator::generatePacket(100, 1, 65530, buffer);

    // Check Wheel Time bytes (Index 8-9)
    // Expected 0xFFF4 -> F4 FF
    TEST_ASSERT_EQUAL_HEX8(0xF4, buffer[8]);
    TEST_ASSERT_EQUAL_HEX8(0xFF, buffer[9]);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_ble_packet_structure);
    RUN_TEST(test_ble_packet_overflow_handling);
    return UNITY_END();
}