#ifndef NATIVE_TEST
//
// Created by leanid on 26.1.24.
//

#include "BLECyclingPowerService.h"

#include "ble_constants.h"

#include "Cadence.h"

#include <Arduino.h>

#include <NimBLEDevice.h>

#include "BlePacketGenerator.h"

static constexpr uint16_t BATTERY_SERVICE_UUID = 0x180F;

BLEServer *powerServer = nullptr;

BLECharacteristic *pCharacteristic = nullptr;

BLECharacteristic *pCharacteristicBatteryLevel = nullptr;

static bool deviceConnected = false;

static bool oldDeviceConnected = false;



class MyServerCallbacks final : public BLEServerCallbacks

{

  auto onConnect(BLEServer *pServer, NimBLEConnInfo &connInfo) -> void override {
    Serial.print("onConnect(");
    deviceConnected = true;

    BLEDevice::startAdvertising();
  };

  void onDisconnect(BLEServer *pServer, NimBLEConnInfo &connInfo, int reason) override

  {
    Serial.print("onDisconnect(");

    deviceConnected = false;
  }
};

void BLECyclingPowerService::setup() {
    setup_BLE_server_multiconnect_NimBLE();
}

void BLECyclingPowerService::update(uint16_t currentPower, const Cadence &cadence) {
    loop_BLE_server_multiconnect_NimBLE(currentPower, cadence);
}

void BLECyclingPowerService::setup_BLE_server_multiconnect_NimBLE()

{

  NimBLEDevice::init("CX6");

  // Create the BLE Server

  powerServer = BLEDevice::createServer();

  powerServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = powerServer->createService(NimBLEUUID(CYCLING_POWER_SERVICE_UUID));

  pCharacteristic = pService->createCharacteristic(NimBLEUUID(CYCLING_POWER_MEASUREMENT_CHAR_UUID),

                                                   NIMBLE_PROPERTY::NOTIFY);


  BLECharacteristic *pCharCPSFeature = pService->createCharacteristic(NimBLEUUID(CYCLING_POWER_FEATURE_CHAR_UUID),

                                                                      NIMBLE_PROPERTY::READ);

  pCharCPSFeature->setValue(CPF_CRANK_REVOLUTION_DATA_SUPPORTED | CPF_WHEEL_REVOLUTION_DATA_SUPPORTED);




  BLECharacteristic *pCharSensor_location = pService->createCharacteristic(

      NimBLEUUID(SENSOR_LOCATION_CHAR_UUID), NIMBLE_PROPERTY::READ);

  constexpr uint8_t pCharSensor_locationValue = SENSOR_LOCATION_CHAIN_RING;

  pCharSensor_location->setValue(&pCharSensor_locationValue, sizeof(pCharSensor_locationValue));

  pService->start();

  // Start advertising

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();

  pAdvertising->addServiceUUID(NimBLEUUID(CYCLING_POWER_SERVICE_UUID));

  BLEService *pServiceBattery = powerServer->createService(NimBLEUUID(BATTERY_SERVICE_UUID));

  pCharacteristicBatteryLevel = pServiceBattery->createCharacteristic((uint16_t)0x2A19, READ | NOTIFY);

  pServiceBattery->start();

  pAdvertising->addServiceUUID(NimBLEUUID(BATTERY_SERVICE_UUID));

  pAdvertising->setAppearance(/*ESP_BLE_APPEARANCE_CYCLING_POWER*/ 0x0484);

  BLEDevice::startAdvertising();
}


void BLECyclingPowerService::loop_BLE_server_multiconnect_NimBLE(const uint16_t currentPower, const Cadence &cadence)

{

  // notify changed value

  if (deviceConnected)

  {

    // float speed = ((float)freq.cadence() * 60) / 163.9;

    // Serial.print("speed ");

    // Serial.println(speed);

    // Serial.print("speed 2 ");

    // Serial.println(6.15 * freq.cadence() * 60 / 1000);

    // Serial.print("total strokes ");

    // Serial.println(freq.totalRevs());

    // Serial.print("total m ");

    // Serial.println(freq.totalRevs() * 6.15);

    // 160 strokes = 1km

    // call this frequently

     uint32_t total_revs = cadence.totalRevs();

    // Serial.println(freq.getGattLastCrankRevolutionTimestamp());

    uint8_t payload[20];
    size_t const generated_packet_size = BlePacketGenerator::generatePacket(currentPower, total_revs, cadence.getGattLastCrankRevolutionTimestamp(), payload);
    pCharacteristic->setValue(payload, generated_packet_size);
    pCharacteristic->notify();

    pCharacteristicBatteryLevel->setValue(79);

    pCharacteristicBatteryLevel->notify();
  }


  // disconnecting

  if (!deviceConnected && oldDeviceConnected)

  {

    powerServer->startAdvertising(); // restart advertising

    oldDeviceConnected = deviceConnected;
  }

  // connecting

  if (deviceConnected && !oldDeviceConnected)

  {

    // do stuff here on connecting

    oldDeviceConnected = deviceConnected;
  }
}
#endif