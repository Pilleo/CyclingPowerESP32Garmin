#include <unity.h>
#include "../../src/BLECyclingPowerService.h"
#include "MockBleStackAdapter.h"
#include "../../src/Cadence.h" // Needed for updateData signature, though we pass primitives now?
// Actually updateData takes primitives now.

MockBleStackAdapter* mockStack;
BLECyclingPowerService* service;

void setUp() {
    mockStack = new MockBleStackAdapter();
    service = new BLECyclingPowerService(*mockStack);
    service->start();
}

void tearDown() {
    delete service;
    delete mockStack;
}

void test_re_advertising_on_disconnect() {
    // 1. Simulate Connection
    mockStack->simulateConnect();
    TEST_ASSERT_TRUE(service->isConnected());

    // Latch the connection state (sets _oldDeviceConnected = true)
    service->updateData(100, 60, 1000);

    // Reset advertising flag (it might be called on connect too depending on logic)
    mockStack->advertisingStarted = false;

    // 2. Simulate Disconnect
    mockStack->simulateDisconnect();
    TEST_ASSERT_FALSE(service->isConnected());

    // 3. Trigger update loop
    // The logic "if (!deviceConnected && oldDeviceConnected)" runs inside updateData
    // We need to call updateData to trigger the state check.
    service->updateData(100, 60, 1000);

    // 4. Assert Advertising Restarted
    TEST_ASSERT_TRUE(mockStack->advertisingStarted);
}

void test_battery_level_update() {
    // 1. Connect
    mockStack->simulateConnect();
    
    // 2. Update Data
    service->updateData(200, 90, 2000);
    
    // 3. Assert Battery Level Set and Notified
    // UUID for Battery Level is "2A19"
    TEST_ASSERT_TRUE(mockStack->wasNotified("2A19"));

    std::vector<uint8_t> val = mockStack->getValue("2A19");
    TEST_ASSERT_EQUAL(1, val.size());
    TEST_ASSERT_EQUAL(79, val[0]);
}

void test_power_measurement_notification() {
    // 1. Connect
    mockStack->simulateConnect();

    // 2. Update Data
    uint16_t power = 250;
    uint32_t revs = 100;
    uint16_t timestamp = 5000;
    service->updateData(power, revs, timestamp);

    // 3. Assert Power Measurement Notified
    // UUID for Power Measurement is "2A63"
    TEST_ASSERT_TRUE(mockStack->wasNotified("2A63"));

    std::vector<uint8_t> val = mockStack->getValue("2A63");
    TEST_ASSERT_NOT_EQUAL(0, val.size());
    // We could verify packet content here if we wanted to test BlePacketGenerator integration
}

void test_power_feature_reports_single_sensor_with_wheel_and_crank_data() {
    const std::vector<uint8_t> value = mockStack->getValue("2A65");
    const uint8_t expected[] = {0x0C, 0x00, 0x10, 0x00};

    TEST_ASSERT_EQUAL(sizeof(expected), value.size());
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, value.data(), sizeof(expected));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_re_advertising_on_disconnect);
    RUN_TEST(test_battery_level_update);
    RUN_TEST(test_power_measurement_notification);
    RUN_TEST(test_power_feature_reports_single_sensor_with_wheel_and_crank_data);
    return UNITY_END();
}
