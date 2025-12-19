#ifndef NATIVE_TEST
//
// Created by leanid on 26.1.24.
//

#include "BLECyclingPowerService.h"
#include "ble_constants.h"
#include "BlePacketGenerator.h"

static constexpr const char* BATTERY_SERVICE_UUID_STR = "180F";
static constexpr const char* BATTERY_LEVEL_CHAR_UUID_STR = "2A19";

// Convert numeric constants to strings for the adapter
// Note: In a real implementation, we might want the adapter to handle numeric UUIDs too.
// For now, let's assume we pass strings or the adapter handles it.
// The constants in ble_constants.h are likely NimBLEUUID objects or strings?
// Let's check ble_constants.h if possible, but assuming they are strings or convertible.
// Actually, NimBLEUUID constructor takes string.
// Let's define string versions of the constants used.

static constexpr const char* CYCLING_POWER_SERVICE_UUID_STR = "1818"; // 0x1818
static constexpr const char* CYCLING_POWER_MEASUREMENT_CHAR_UUID_STR = "2A63"; // 0x2A63
static constexpr const char* CYCLING_POWER_FEATURE_CHAR_UUID_STR = "2A65"; // 0x2A65
static constexpr const char* SENSOR_LOCATION_CHAR_UUID_STR = "2A5D"; // 0x2A5D

BLECyclingPowerService::BLECyclingPowerService(IBleStackAdapter& bleStack)
    : _bleStack(bleStack) {}

void BLECyclingPowerService::start() {
    _bleStack.init("CX6");
    _bleStack.setCallbacks(this);

    // Power Service
    _bleStack.createService(CYCLING_POWER_SERVICE_UUID_STR);

    _hPowerMeas = _bleStack.createCharacteristic(
        CYCLING_POWER_SERVICE_UUID_STR,
        CYCLING_POWER_MEASUREMENT_CHAR_UUID_STR,
        IBleStackAdapter::PROP_NOTIFY
    );

    _hFeature = _bleStack.createCharacteristic(
        CYCLING_POWER_SERVICE_UUID_STR,
        CYCLING_POWER_FEATURE_CHAR_UUID_STR,
        IBleStackAdapter::PROP_READ
    );
    // Set Feature Value
    uint32_t featureVal = CPF_CRANK_REVOLUTION_DATA_SUPPORTED | CPF_WHEEL_REVOLUTION_DATA_SUPPORTED;
    _bleStack.setCharacteristicValue(_hFeature, (uint8_t*)&featureVal, sizeof(featureVal)); // Careful with endianness/size

    _hSensorLoc = _bleStack.createCharacteristic(
        CYCLING_POWER_SERVICE_UUID_STR,
        SENSOR_LOCATION_CHAR_UUID_STR,
        IBleStackAdapter::PROP_READ
    );
    uint8_t locVal = SENSOR_LOCATION_CHAIN_RING;
    _bleStack.setCharacteristicValue(_hSensorLoc, &locVal, 1);

    _bleStack.startService(CYCLING_POWER_SERVICE_UUID_STR);

    // Battery Service
    _bleStack.createService(BATTERY_SERVICE_UUID_STR);
    _hBattery = _bleStack.createCharacteristic(
        BATTERY_SERVICE_UUID_STR,
        BATTERY_LEVEL_CHAR_UUID_STR,
        IBleStackAdapter::PROP_READ | IBleStackAdapter::PROP_NOTIFY
    );
    _bleStack.startService(BATTERY_SERVICE_UUID_STR);

    // Advertising
    _bleStack.addServiceToAdvertising(CYCLING_POWER_SERVICE_UUID_STR);
    _bleStack.addServiceToAdvertising(BATTERY_SERVICE_UUID_STR);
    _bleStack.setAppearance(0x0484); // Cycling Power
    _bleStack.startAdvertising();
}

void BLECyclingPowerService::updateData(uint16_t power, uint32_t revs, uint16_t timestamp) {
    // Handle connection state changes logic
    if (!_deviceConnected && _oldDeviceConnected) {
        _bleStack.startAdvertising();
        _oldDeviceConnected = _deviceConnected;
    }

    if (_deviceConnected && !_oldDeviceConnected) {
        _oldDeviceConnected = _deviceConnected;
    }

    if (_deviceConnected) {
        uint8_t payload[20];
        size_t const generated_packet_size = BlePacketGenerator::generatePacket(power, revs, timestamp, payload);

        _bleStack.setCharacteristicValue(_hPowerMeas, payload, generated_packet_size);
        _bleStack.notify(_hPowerMeas);

        _bleStack.setCharacteristicValue(_hBattery, 79);
        _bleStack.notify(_hBattery);
    }
}

bool BLECyclingPowerService::isConnected() {
    return _deviceConnected;
}

void BLECyclingPowerService::onConnect() {
    _deviceConnected = true;
    // Original code called startAdvertising on connect too?
    // "BLEDevice::startAdvertising();" was in onConnect.
    // This is unusual (usually you stop advertising), but maybe for multi-connect?
    // Let's keep it if that was the intent.
    _bleStack.startAdvertising();
}

void BLECyclingPowerService::onDisconnect() {
    _deviceConnected = false;
}

#endif
