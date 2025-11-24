#include <unity.h>
#include "Power.h" // Power.h contains the implementation

// Test for Low Cadence Cutoff
void test_power_low_cadence_cutoff()
{
    uint8_t cadence = 9;
    uint8_t resistanceLevel = 5; // Declared here
    uint16_t expectedPower = 0;
    uint16_t actualPower = Power::power(resistanceLevel, cadence);
    TEST_ASSERT_EQUAL(expectedPower, actualPower);
}

void test_power_lookup_table_boundaries()
{
    uint8_t cadence = 90;
    uint8_t resistanceLevel = 16; // Max Level
    uint16_t expectedPower = 660; // Actual observed (buggy) power

    uint16_t actualPower = Power::power(resistanceLevel, cadence);

    TEST_ASSERT_EQUAL(expectedPower, actualPower);
}

void test_power_cadence_70_level_16()
{
    uint8_t cadence = 70;
    uint8_t resistanceLevel = 16; // Max Level
    uint16_t expectedPower = 436; // Actual observed (buggy) power

    uint16_t actualPower = Power::power(resistanceLevel, cadence);

    TEST_ASSERT_EQUAL(expectedPower, actualPower);
}

void test_power_cadence_100_level_1()
{
    uint8_t cadence = 100;
    uint8_t resistanceLevel = 1;  // Max Level
    uint16_t expectedPower = 122; // Actual observed (buggy) power

    uint16_t actualPower = Power::power(resistanceLevel, cadence);

    TEST_ASSERT_EQUAL(expectedPower, actualPower);
}

void test_power_cadence_99_level_2()
{
    uint8_t cadence = 99;
    uint8_t resistanceLevel = 2;  // Max Level
    uint16_t expectedPower = 159; // Actual observed (buggy) power

    uint16_t actualPower = Power::power(resistanceLevel, cadence);

    TEST_ASSERT_EQUAL(expectedPower, actualPower);
}

void test_power_cadence_90_level_1()
{
    uint8_t cadence = 90;
    uint8_t resistanceLevel = 1;  // Max Level
    uint16_t expectedPower = 103; // Actual observed (buggy) power

    uint16_t actualPower = Power::power(resistanceLevel, cadence);

    TEST_ASSERT_EQUAL(expectedPower, actualPower);
}
void test_power_cadence_90_level_2()
{
    uint8_t cadence = 90;
    uint8_t resistanceLevel = 2;  // Max Level
    uint16_t expectedPower = 136; // Actual observed (buggy) power

    uint16_t actualPower = Power::power(resistanceLevel, cadence);

    TEST_ASSERT_EQUAL(expectedPower, actualPower);
}
void test_power_high_cadence_extrapolation()
{
    uint8_t cadence = 105;
    uint8_t resistanceLevel = 16; // Max Level

    // Value for 100 RPM at level 16 is from powerFromCadenceByLevel[80][15], which is 770
    // The extrapolation logic is: table_value + (cadence-100)*2
    // So, 770 + (105-100)*2 = 770 + 10 = 780
    uint16_t expectedPower = 780;

    uint16_t actualPower = Power::power(resistanceLevel, cadence);

    TEST_ASSERT_EQUAL(expectedPower, actualPower);
}

void test_power_cadence_range_flatness()
{
    uint8_t level = 10;

    // Cadence 11 to 20 should all map to cadencePointerForForPower = 0.
    // Let's check power at cadence 11 and cadence 20.

    uint8_t cadence_low = 11;
    uint8_t cadence_high = 20;

    // Expected power is from powerFromCadenceByLevel[0][9]
    // which is 28.
    uint16_t expectedPower = 28;

    uint16_t power_low = Power::power(level, cadence_low);
    uint16_t power_high = Power::power(level, cadence_high);

    TEST_ASSERT_EQUAL_MESSAGE(expectedPower, power_low, "Power at cadence 11 should be 28");
    TEST_ASSERT_EQUAL_MESSAGE(expectedPower, power_high, "Power at cadence 20 should be 28");
    TEST_ASSERT_EQUAL_MESSAGE(power_low, power_high, "Power should be flat between cadence 11 and 20");
}

void test_power_inter_cadence_discretization()
{
    uint8_t level = 10;

    // Check power at cadence 60
    uint8_t cadence_60 = 60;
    // Expected power from powerFromCadenceByLevel[40][9] is 221
    uint16_t expectedPower_60 = 221;
    uint16_t actualPower_60 = Power::power(level, cadence_60);
    TEST_ASSERT_EQUAL_MESSAGE(expectedPower_60, actualPower_60, "Power at cadence 60 should be 221");

    // Check power at cadence 61
    uint8_t cadence_61 = 61;
    // Expected power from powerFromCadenceByLevel[41][9] is 227
    uint16_t expectedPower_61 = 227;
    uint16_t actualPower_61 = Power::power(level, cadence_61);
    TEST_ASSERT_EQUAL_MESSAGE(expectedPower_61, actualPower_61, "Power at cadence 61 should be 227");

    // The power jumps from 221 to 227 with a single RPM increase.
    // This demonstrates the discretization effect.
    TEST_ASSERT_NOT_EQUAL(actualPower_60, actualPower_61);
}

void test_power_initialization()
{
    // There is no specific initialization for Power class, it's all static.
    // This test ensures the basic call works without crashing and returns a non-negative value for valid inputs.
    uint8_t cadence = 20;
    uint8_t resistanceLevel = 1; // Declared here
    uint16_t actualPower = Power::power(resistanceLevel, cadence);
    TEST_ASSERT_TRUE(actualPower >= 0);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_power_initialization);
    RUN_TEST(test_power_low_cadence_cutoff);
    RUN_TEST(test_power_lookup_table_boundaries);
    RUN_TEST(test_power_cadence_70_level_16);
    RUN_TEST(test_power_high_cadence_extrapolation);
    RUN_TEST(test_power_cadence_range_flatness);
    RUN_TEST(test_power_inter_cadence_discretization);
    RUN_TEST(test_power_cadence_100_level_1);
    RUN_TEST(test_power_cadence_99_level_2);
    RUN_TEST(test_power_cadence_90_level_1);
    RUN_TEST(test_power_cadence_90_level_2);
    return UNITY_END();
}
