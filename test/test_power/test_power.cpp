#include <unity.h>
#include "Power.h" // Power.h contains the implementation

// Test for Low Cadence Cutoff
void test_power_low_cadence_cutoff()
{
    float cadence = 9.0f;
    uint8_t resistanceLevel = 5; // Declared here
    uint16_t expectedPower = 0;
    uint16_t actualPower = Power::power(resistanceLevel, cadence);
    TEST_ASSERT_EQUAL(expectedPower, actualPower);
}

void test_power_high_cadence_extrapolation()
{
    float cadence = 105.0f;
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

    float cadence_low = 11.0f;
    float cadence_high = 20.0f;

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
    float cadence_60 = 60.0f;
    // Expected power from powerFromCadenceByLevel[40][9] is 221
    uint16_t expectedPower_60 = 221;
    uint16_t actualPower_60 = Power::power(level, cadence_60);
    TEST_ASSERT_EQUAL_MESSAGE(expectedPower_60, actualPower_60, "Power at cadence 60 should be 221");

    // Check power at cadence 61
    float cadence_61 = 61.0f;
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
    float cadence = 20.0f;
    uint8_t resistanceLevel = 1; // Declared here
    uint16_t actualPower = Power::power(resistanceLevel, cadence);
    TEST_ASSERT_TRUE(actualPower >= 0);
}

struct PowerTestCase
{
    float cadence;
    uint8_t resistanceLevel;
    uint16_t expectedPower;
    const char *message;
};

void test_power_parametrized()
{
    PowerTestCase testCases[] = {
        {90.0f, 16, 660, "Cadence 90, Level 16"},
        {70.0f, 16, 436, "Cadence 70, Level 16"},
        {100.0f, 1, 122, "Cadence 100, Level 1"},
        {99.0f, 2, 159, "Cadence 99, Level 2"},
        {90.0f, 1, 103, "Cadence 90, Level 1"},
        {90.0f, 2, 136, "Cadence 90, Level 2"}};

    for (unsigned int i = 0; i < sizeof(testCases) / sizeof(PowerTestCase); ++i)
    {
        uint16_t actualPower = Power::power(testCases[i].resistanceLevel, testCases[i].cadence);
        TEST_ASSERT_EQUAL_UINT16_MESSAGE(testCases[i].expectedPower, actualPower, testCases[i].message);
    }
}

struct PowerInterpolationTestCase
{
    float cadence;
    uint8_t resistanceLevel;
    uint16_t expectedPower;
    const char *message;
};

void test_power_interpolation_parametrized()
{
    PowerInterpolationTestCase testCases[] = {
        {90.5f, 2, 137, "Cadence 90.5, Level 2"},
        {99.5f, 2, 161, "Cadence 99.5, Level 2"},
        {70.5f, 16, 442, "Cadence 70.5, Level 16"},
        {70.9f, 16, 446, "Cadence 70.5, Level 16"}

    };

    for (unsigned int i = 0; i < sizeof(testCases) / sizeof(PowerInterpolationTestCase); ++i)
    {
        uint16_t actualPower = Power::power(testCases[i].resistanceLevel, testCases[i].cadence);
        TEST_ASSERT_EQUAL_UINT16_MESSAGE(testCases[i].expectedPower, actualPower, testCases[i].message);
    }
}
/**
 * Test 3.2.1: Resistance Level Bounds Safety
 * Verifies that invalid resistance levels (0 or >16) are clamped or handled safely
 * to prevent array out-of-bounds access.
 */
void test_power_level_bounds_safety() {
    float standardCadence = 90.0f;

    // Case A: Level 0
    // Current logic: (resistanceLevel > 0 && resistanceLevel <= 16) ? (resistanceLevel - 1) : 0;
    // So Level 0 maps to Level 1 (Index 0).
    uint16_t powerAtLevel0 = Power::power(0, standardCadence);
    uint16_t powerAtLevel1 = Power::power(1, standardCadence);
    TEST_ASSERT_EQUAL_MESSAGE(powerAtLevel1, powerAtLevel0, "Level 0 should map to Level 1 safety defaults");

    // Case B: Level 17 (Out of bounds)
    // Logic: Clamps to Index 0 (Level 1) due to the ternary check failing?
    // Check code: `(resistanceLevel > 0 && resistanceLevel <= 16) ? ... : 0;`
    // Yes, invalid levels default to Index 0 (Level 1).
    uint16_t powerAtLevel17 = Power::power(17, standardCadence);
    TEST_ASSERT_EQUAL_MESSAGE(powerAtLevel1, powerAtLevel17, "Level 17 should map to Level 1 safety defaults");

    // Case C: Max Byte (255)
    uint16_t powerAtMax = Power::power(255, standardCadence);
    TEST_ASSERT_EQUAL(powerAtLevel1, powerAtMax);
}

/**
 * Test 3.2.2: Negative Cadence Input
 * Ensures that negative float values for cadence don't cause crashes or wild values.
 */
void test_power_negative_cadence() {
    float negCadence = -50.0f;

    // Code logic check: `if (cadence < 10.0F) return 0;`
    // -50 is < 10, so it should return 0.
    uint16_t power = Power::power(5, negCadence);
    TEST_ASSERT_EQUAL(0, power);
}
int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_power_initialization);
    RUN_TEST(test_power_low_cadence_cutoff);
    RUN_TEST(test_power_high_cadence_extrapolation);
    RUN_TEST(test_power_cadence_range_flatness);
    RUN_TEST(test_power_inter_cadence_discretization);
    RUN_TEST(test_power_parametrized);
    RUN_TEST(test_power_interpolation_parametrized);
    RUN_TEST(test_power_level_bounds_safety);
    RUN_TEST(test_power_negative_cadence);

    return UNITY_END();
}
