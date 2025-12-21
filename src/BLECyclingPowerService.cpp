//
// Created by leanid on 26.1.24.
//

#include "BLECyclingPowerService.h"
#include "ble_constants.h"
#include "BlePacketGenerator.h"

// Service and Characteristic UUIDs
static constexpr const char* BATTERY_SERVICE_UUID_STR = "180F";
static constexpr const char* BATTERY_LEVEL_CHAR_UUID_STR = "2A19";
static constexpr const char* CYCLING_POWER_SERVICE_UUID_STR = "1818";
static constexpr const char* CYCLING_POWER_MEASUREMENT_CHAR_UUID_STR = "2A63";
static constexpr const char* CYCLING_POWER_FEATURE_CHAR_UUID_STR = "2A65";
static constexpr const char* SENSOR_LOCATION_CHAR_UUID_STR = "2A5D";

// Appearance
static constexpr uint16_t APPEARANCE_CYCLING_POWER = 0x0484;

// Battery level (dummy value for now)
static constexpr uint8_t DUMMY_BATTERY_LEVEL = 79;

BLECyclingPowerService::BLECyclingPowerService(IBleStackAdapter& bleStack)
    : _bleStack(bleStack) {}

void BLECyclingPowerService::start() {
    _bleStack.init("CX6");
    _bleStack.setCallbacks(this);

    setupPowerService();
    setupBatteryService();
    setupAdvertising();

    _bleStack.startAdvertising();
}

void BLECyclingPowerService::setupPowerService() {
    _bleStack.createService(CYCLING_POWER_SERVICE_UUID_STR);

    _powerMeasurementCharacteristic = _bleStack.createCharacteristic(
        CYCLING_POWER_SERVICE_UUID_STR,
        CYCLING_POWER_MEASUREMENT_CHAR_UUID_STR,
        IBleStackAdapter::PROP_NOTIFY);

    _featureCharacteristic = _bleStack.createCharacteristic(
        CYCLING_POWER_SERVICE_UUID_STR,
        CYCLING_POWER_FEATURE_CHAR_UUID_STR,
        IBleStackAdapter::PROP_READ);
    uint32_t featureVal = CPF_CRANK_REVOLUTION_DATA_SUPPORTED | CPF_WHEEL_REVOLUTION_DATA_SUPPORTED;
    _bleStack.setCharacteristicValue(_featureCharacteristic, reinterpret_cast<uint8_t*>(&featureVal), sizeof(featureVal));

    _sensorLocationCharacteristic = _bleStack.createCharacteristic(
        CYCLING_POWER_SERVICE_UUID_STR,
        SENSOR_LOCATION_CHAR_UUID_STR,
        IBleStackAdapter::PROP_READ);
    uint8_t locVal = SENSOR_LOCATION_CHAIN_RING;
    _bleStack.setCharacteristicValue(_sensorLocationCharacteristic, &locVal, sizeof(locVal));

    _bleStack.startService(CYCLING_POWER_SERVICE_UUID_STR);
}

void BLECyclingPowerService::setupBatteryService() {
    _bleStack.createService(BATTERY_SERVICE_UUID_STR);
    _batteryLevelCharacteristic = _bleStack.createCharacteristic(
        BATTERY_SERVICE_UUID_STR,
        BATTERY_LEVEL_CHAR_UUID_STR,
        IBleStackAdapter::PROP_READ | IBleStackAdapter::PROP_NOTIFY);
    _bleStack.startService(BATTERY_SERVICE_UUID_STR);
}

void BLECyclingPowerService::setupAdvertising() {
    _bleStack.addServiceToAdvertising(CYCLING_POWER_SERVICE_UUID_STR);
    _bleStack.addServiceToAdvertising(BATTERY_SERVICE_UUID_STR);
    _bleStack.setAppearance(APPEARANCE_CYCLING_POWER);
}

void BLECyclingPowerService::updateData(uint16_t power, uint32_t totalRevolutions, uint16_t crankEventTime) {
    if (!_deviceConnected) {
        return;
    }

    uint8_t payload[BlePacketGenerator::MAX_PACKET_SIZE];
    size_t packetSize = BlePacketGenerator::generatePacket(power, totalRevolutions, crankEventTime, payload);

    if (packetSize > 0) {
        _bleStack.setCharacteristicValue(_powerMeasurementCharacteristic, payload, packetSize);
        _bleStack.notify(_powerMeasurementCharacteristic);

        _bleStack.setCharacteristicValue(_batteryLevelCharacteristic, DUMMY_BATTERY_LEVEL);
        _bleStack.notify(_batteryLevelCharacteristic);
    }
}

bool BLECyclingPowerService::isConnected() {
    return _deviceConnected;
}

void BLECyclingPowerService::onConnect() {
    _deviceConnected = true;
    // Restart advertising to allow other devices to connect (if supported by the stack)
    _bleStack.startAdvertising();
}

void BLECyclingPowerService::onDisconnect() {
    _deviceConnected = false;
    // The stack should handle cleanup, but we restart advertising to be discoverable again.
    _bleStack.startAdvertising();
}
