#include <unity.h>
#include "../../src/BLECyclingPowerService.h"
#include "MockBleStackAdapter.h"
#include "../../src/Cadence.h" // Needed for updateData signature, though we pass primitives now?
#include <algorithm>
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

void test_csc_service_is_discoverable_and_configured() {
    TEST_ASSERT_NOT_EQUAL(
        mockStack->createdServices.end(),
        std::find(mockStack->createdServices.begin(),
                  mockStack->createdServices.end(), "1816"));
    TEST_ASSERT_NOT_EQUAL(
        mockStack->advertisedServices.end(),
        std::find(mockStack->advertisedServices.begin(),
                  mockStack->advertisedServices.end(), "1816"));

    const auto* measurement = mockStack->findCharacteristic("1816", "2A5B");
    const auto* feature = mockStack->findCharacteristic("1816", "2A5C");
    const auto* location = mockStack->findCharacteristic("1816", "2A5D");

    TEST_ASSERT_NOT_NULL(measurement);
    TEST_ASSERT_EQUAL_UINT32(IBleStackAdapter::PROP_NOTIFY,
                             measurement->properties);
    TEST_ASSERT_NOT_NULL(feature);
    TEST_ASSERT_EQUAL_UINT32(IBleStackAdapter::PROP_READ, feature->properties);
    const uint8_t expectedFeature[] = {0x03, 0x00};
    TEST_ASSERT_EQUAL(sizeof(expectedFeature), feature->value.size());
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expectedFeature, feature->value.data(),
                                 sizeof(expectedFeature));
    TEST_ASSERT_NOT_NULL(location);
    TEST_ASSERT_EQUAL_UINT32(IBleStackAdapter::PROP_READ, location->properties);
}

void test_advertises_speed_and_cadence_sensor_appearance() {
    TEST_ASSERT_EQUAL_HEX16(0x0485, mockStack->appearance);
}

void test_update_notifies_power_and_csc_measurements() {
    mockStack->simulateConnect();
    service->updateData(250, 100, 5000);

    TEST_ASSERT_TRUE(mockStack->wasNotified("2A63"));
    TEST_ASSERT_TRUE(mockStack->wasNotified("2A5B"));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_re_advertising_on_disconnect);
    RUN_TEST(test_battery_level_update);
    RUN_TEST(test_power_measurement_notification);
    RUN_TEST(test_power_feature_reports_single_sensor_with_wheel_and_crank_data);
    RUN_TEST(test_csc_service_is_discoverable_and_configured);
    RUN_TEST(test_advertises_speed_and_cadence_sensor_appearance);
    RUN_TEST(test_update_notifies_power_and_csc_measurements);
    return UNITY_END();
}
