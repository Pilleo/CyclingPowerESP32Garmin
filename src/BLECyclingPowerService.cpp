//
// Created by leanid on 26.1.24.
//

#include "BLECyclingPowerService.h"
#include "BlePacketGenerator.h"
#include "CscPacketGenerator.h"
#include "ble_constants.h"
#include "config.h"
#include <array>
#include <cstring>

// Service and Characteristic UUIDs
static constexpr const char *BATTERY_SERVICE_UUID_STR = "180F";
static constexpr const char *BATTERY_LEVEL_CHAR_UUID_STR = "2A19";
static constexpr const char *CYCLING_POWER_SERVICE_UUID_STR = "1818";
static constexpr const char *CYCLING_POWER_MEASUREMENT_CHAR_UUID_STR = "2A63";
static constexpr const char *CYCLING_POWER_FEATURE_CHAR_UUID_STR = "2A65";
static constexpr const char *CP_CONTROL_POINT_CHAR_UUID_STR = "2A66";
static constexpr const char *SENSOR_LOCATION_CHAR_UUID_STR = "2A5D";
static constexpr const char *CYCLING_SPEED_CADENCE_SERVICE_UUID_STR = "1816";
static constexpr const char *CSC_MEASUREMENT_CHAR_UUID_STR = "2A5B";
static constexpr const char *CSC_FEATURE_CHAR_UUID_STR = "2A5C";
static constexpr const char *SC_CONTROL_POINT_CHAR_UUID_STR = "2A55";

// Appearance
static constexpr uint16_t APPEARANCE_CYCLING_SPEED_AND_CADENCE = 0x0485;

// Battery level (dummy value for now)
static constexpr uint8_t DUMMY_BATTERY_LEVEL = 79;
static constexpr uint8_t SC_RESPONSE_CODE = 0x10;
static constexpr uint8_t SC_SET_CUMULATIVE_VALUE = 0x01;
static constexpr uint8_t SC_RESPONSE_SUCCESS = 0x01;
static constexpr uint8_t SC_RESPONSE_OPCODE_NOT_SUPPORTED = 0x02;
static constexpr uint8_t SC_RESPONSE_INVALID_PARAMETER = 0x03;

BLECyclingPowerService::BLECyclingPowerService(
    IBleStackAdapter &bleStack) noexcept
    : _bleStack(bleStack) {}

void BLECyclingPowerService::start()
{
  _bleStack.init(BLE_DEVICE_NAME);
  _bleStack.setCallbacks(this);

  setupPowerService();
  setupCscService();
  setupBatteryService();
  setupAdvertising();

  _bleStack.startAdvertising();
}

void BLECyclingPowerService::setupCscService()
{
  _bleStack.createService(CYCLING_SPEED_CADENCE_SERVICE_UUID_STR);

  _cscMeasurementCharacteristic = _bleStack.createCharacteristic(
      CYCLING_SPEED_CADENCE_SERVICE_UUID_STR, CSC_MEASUREMENT_CHAR_UUID_STR,
      IBleStackAdapter::PROP_NOTIFY);
  _cscFeatureCharacteristic = _bleStack.createCharacteristic(
      CYCLING_SPEED_CADENCE_SERVICE_UUID_STR, CSC_FEATURE_CHAR_UUID_STR,
      IBleStackAdapter::PROP_READ);
  constexpr uint16_t cscFeatureValue =
      CSCF_WHEEL_REVOLUTION_DATA_SUPPORTED |
      CSCF_CRANK_REVOLUTION_DATA_SUPPORTED;
  _bleStack.setCharacteristicValue(
      _cscFeatureCharacteristic,
      reinterpret_cast<const uint8_t *>(&cscFeatureValue),
      sizeof(cscFeatureValue));

  _cscSensorLocationCharacteristic = _bleStack.createCharacteristic(
      CYCLING_SPEED_CADENCE_SERVICE_UUID_STR, SENSOR_LOCATION_CHAR_UUID_STR,
      IBleStackAdapter::PROP_READ);
  constexpr uint8_t cscLocation = SENSOR_LOCATION_CHAIN_RING;
  _bleStack.setCharacteristicValue(_cscSensorLocationCharacteristic,
                                   &cscLocation, sizeof(cscLocation));

  _cscControlPointCharacteristic = _bleStack.createCharacteristic(
      CYCLING_SPEED_CADENCE_SERVICE_UUID_STR,
      SC_CONTROL_POINT_CHAR_UUID_STR,
      IBleStackAdapter::PROP_WRITE | IBleStackAdapter::PROP_INDICATE);
  _bleStack.setCharacteristicCallbacks(_cscControlPointCharacteristic, this);

  _bleStack.startService(CYCLING_SPEED_CADENCE_SERVICE_UUID_STR);
}

void BLECyclingPowerService::setupPowerService()
{
  _bleStack.createService(CYCLING_POWER_SERVICE_UUID_STR);

  _powerMeasurementCharacteristic = _bleStack.createCharacteristic(
      CYCLING_POWER_SERVICE_UUID_STR, CYCLING_POWER_MEASUREMENT_CHAR_UUID_STR,
      IBleStackAdapter::PROP_NOTIFY);

  _featureCharacteristic = _bleStack.createCharacteristic(
      CYCLING_POWER_SERVICE_UUID_STR, CYCLING_POWER_FEATURE_CHAR_UUID_STR,
      IBleStackAdapter::PROP_READ);

  constexpr uint32_t powerFeatureVal =
      CPF_CRANK_REVOLUTION_DATA_SUPPORTED |
      CPF_WHEEL_REVOLUTION_DATA_SUPPORTED |
      CPF_DISTRIBUTED_SYSTEM_NOT_SUPPORTED;
  _bleStack.setCharacteristicValue(_featureCharacteristic,
                                   reinterpret_cast<const uint8_t *>(
                                       &powerFeatureVal),
                                   sizeof(powerFeatureVal));

  _sensorLocationCharacteristic = _bleStack.createCharacteristic(
      CYCLING_POWER_SERVICE_UUID_STR, SENSOR_LOCATION_CHAR_UUID_STR,
      IBleStackAdapter::PROP_READ);
  static constexpr uint8_t locVal = SENSOR_LOCATION_CHAIN_RING;
  _bleStack.setCharacteristicValue(_sensorLocationCharacteristic, &locVal,
                                   sizeof(locVal));

  _cpsControlPointCharacteristic = _bleStack.createCharacteristic(
      CYCLING_POWER_SERVICE_UUID_STR, CP_CONTROL_POINT_CHAR_UUID_STR,
      IBleStackAdapter::PROP_WRITE | IBleStackAdapter::PROP_INDICATE);
  _bleStack.setCharacteristicCallbacks(_cpsControlPointCharacteristic, this);

  _bleStack.startService(CYCLING_POWER_SERVICE_UUID_STR);
}

void BLECyclingPowerService::setupBatteryService()
{
  _bleStack.createService(BATTERY_SERVICE_UUID_STR);
  _batteryLevelCharacteristic = _bleStack.createCharacteristic(
      BATTERY_SERVICE_UUID_STR, BATTERY_LEVEL_CHAR_UUID_STR,
      IBleStackAdapter::PROP_READ | IBleStackAdapter::PROP_NOTIFY);
  _bleStack.startService(BATTERY_SERVICE_UUID_STR);
}

void BLECyclingPowerService::setupAdvertising() const
{
  _bleStack.addServiceToAdvertising(CYCLING_POWER_SERVICE_UUID_STR);
  _bleStack.addServiceToAdvertising(CYCLING_SPEED_CADENCE_SERVICE_UUID_STR);
  _bleStack.addServiceToAdvertising(BATTERY_SERVICE_UUID_STR);
  _bleStack.setAppearance(APPEARANCE_CYCLING_SPEED_AND_CADENCE);
}

void BLECyclingPowerService::updateData(const CyclingTelemetry& telemetry)
{
  if (!_deviceConnected)
  {
    return;
  }

  std::array<uint8_t, BlePacketGenerator::MAX_PACKET_SIZE> payload;

  // Generate and notify for the Cycling Power Service (Power + Cadence)
  _lastBaseCpsWheelRevolutions = telemetry.crankRevolutions *
                                 WHEEL_REVOLUTIONS_PER_CRANK_REVOLUTION;
  const size_t cppPacketSize = BlePacketGenerator::generatePacket(
      telemetry.powerWatts, telemetry.crankRevolutions,
      telemetry.crankEventTime1024, payload.data());
  const uint32_t cpsWheelRevolutions =
      _cpsControlPoint.apply(_lastBaseCpsWheelRevolutions);
  payload[4] = static_cast<uint8_t>(cpsWheelRevolutions);
  payload[5] = static_cast<uint8_t>(cpsWheelRevolutions >> 8U);
  payload[6] = static_cast<uint8_t>(cpsWheelRevolutions >> 16U);
  payload[7] = static_cast<uint8_t>(cpsWheelRevolutions >> 24U);

  _bleStack.setCharacteristicValue(_powerMeasurementCharacteristic,
                                   payload.data(), cppPacketSize);
  _bleStack.notify(_powerMeasurementCharacteristic);

  std::array<uint8_t, CscPacketGenerator::MAX_PACKET_SIZE> cscPayload{};
  _lastBaseCscWheelRevolutions =
      telemetry.crankRevolutions * WHEEL_REVOLUTIONS_PER_CRANK_REVOLUTION;
  const uint32_t cscWheelRevolutions = static_cast<uint32_t>(
      static_cast<int64_t>(_lastBaseCscWheelRevolutions) +
      _cscWheelRevolutionOffset);
  const size_t cscPacketSize = CscPacketGenerator::generatePacketFromCounts(
      cscWheelRevolutions, telemetry.crankEventTime1024,
      static_cast<uint16_t>(telemetry.crankRevolutions),
      telemetry.crankEventTime1024,
      cscPayload.data());
  _bleStack.setCharacteristicValue(_cscMeasurementCharacteristic,
                                   cscPayload.data(), cscPacketSize);
  _bleStack.notify(_cscMeasurementCharacteristic);

  _bleStack.setCharacteristicValue(_batteryLevelCharacteristic,
                                   DUMMY_BATTERY_LEVEL);
  _bleStack.notify(_batteryLevelCharacteristic);
}

auto BLECyclingPowerService::isConnected() -> bool { return _deviceConnected; }

void BLECyclingPowerService::onConnect()
{
  _deviceConnected = true;
  // Restart advertising to allow other devices to connect (if supported by the
  // stack)
  _bleStack.startAdvertising();
}

void BLECyclingPowerService::onDisconnect()
{
  _deviceConnected = false;
  // The stack should handle cleanup, but we restart advertising to be
  // discoverable again.
  _bleStack.startAdvertising();
}

void BLECyclingPowerService::onWrite(void *handle, const uint8_t *data,
                                     size_t length)
{
  if (data == nullptr || length == 0)
  {
    return;
  }
  if (handle == _cpsControlPointCharacteristic)
  {
    const CpsControlPointResponse response = _cpsControlPoint.write(
        data, length, _lastBaseCpsWheelRevolutions);
    _bleStack.setCharacteristicValue(_cpsControlPointCharacteristic,
                                     response.bytes, sizeof(response.bytes));
    _bleStack.indicate(_cpsControlPointCharacteristic);
    return;
  }
  if (handle != _cscControlPointCharacteristic)
  {
    return;
  }
  const uint8_t opcode = data[0];
  uint8_t result = SC_RESPONSE_OPCODE_NOT_SUPPORTED;
  if (opcode == SC_SET_CUMULATIVE_VALUE)
  {
    if (length == 5)
    {
      uint32_t requested;
      std::memcpy(&requested, data + 1, sizeof(requested));
      _cscWheelRevolutionOffset = static_cast<int64_t>(requested) -
                                  static_cast<int64_t>(_lastBaseCscWheelRevolutions);
      result = SC_RESPONSE_SUCCESS;
    }
    else
    {
      result = SC_RESPONSE_INVALID_PARAMETER;
    }
  }
  const uint8_t response[] = {SC_RESPONSE_CODE, opcode, result};
  _bleStack.setCharacteristicValue(_cscControlPointCharacteristic, response,
                                   sizeof(response));
  _bleStack.indicate(_cscControlPointCharacteristic);
}
