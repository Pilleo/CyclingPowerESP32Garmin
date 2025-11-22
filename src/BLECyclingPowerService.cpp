//

// Created by leanid on 26.1.24.

//

#include "BLECyclingPowerService.h"

#include "ble_constants.h"

#include "Cadence.h"

#include <Arduino.h>

#include <NimBLEDevice.h>

static constexpr uint16_t BATTERY_SERVICE_UUID = 0x180F;

BLEServer *powerServer = nullptr;

BLECharacteristic *pCharacteristic = nullptr;

BLECharacteristic *pCharacteristicBatteryLevel = nullptr;

bool deviceConnected = false;

bool oldDeviceConnected = false;

struct __attribute__((__packed__)) CPSMeasurement_t

{

  uint16_t flags;

  uint16_t power; // W

  uint32_t wheel_revs;

  uint16_t wheel_rev_timestamp; // 1/2048 s

  uint16_t crank_revs;

  uint16_t crank_rev_timestamp; // 1/1024 s

  // uint16_t energy; // kJ

} CPSMeasurement;

class MyCallback final : public BLECharacteristicCallbacks

{

  // write performed on the control point characteristic

  void onWrite(BLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override

  {

    const std::string message = pCharacteristic->getValue();

    // std::string - very important, as it doesnt stop when getting a byte of zeros

    Serial.print("write");

    Serial.print("'");

    for (int i = 0; i < message.length(); i++)

      Serial.print(message[i]);

    // print the message byte by byte - any conversion to a normal String terminates it on a byte of zeros

    Serial.print("'"); // for debugging and new messages

    constexpr uint8_t value[3] = {0x80, 50, 0x01};

    // confirmation data, default - 0x02 - Op "Code not supported", no app cares

    pCharacteristic->setValue(value, 3); // response to write

    pCharacteristic->indicate(); // indicate response
  }

  void onRead(BLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override

  {

    Serial.print("onRead(");

    Serial.println(pCharacteristic->getUUID().toString().c_str());
  };
};

class MyServerCallbacks final : public BLEServerCallbacks

{

  void onConnect(BLEServer *pServer, NimBLEConnInfo &connInfo) override

  {

    deviceConnected = true;

    BLEDevice::startAdvertising();
  };

  void onDisconnect(BLEServer *pServer, NimBLEConnInfo &connInfo, int reason) override

  {

    deviceConnected = false;
  }
};

void BLECyclingPowerService::setup_BLE_server_multiconnect_NimBLE()

{

  NimBLEDevice::init("CX6");

  // Create the BLE Server

  powerServer = BLEDevice::createServer();

  powerServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = powerServer->createService(NimBLEUUID(CYCLING_POWER_SERVICE_UUID));

  pCharacteristic = pService->createCharacteristic(NimBLEUUID(CYCLING_POWER_MEASUREMENT_CHAR_UUID),

                                                   NIMBLE_PROPERTY::NOTIFY);

  CPSMeasurement.flags = (CPM_WHEEL_REV_DATA_PRESENT | CPM_CRANK_REV_DATA_PRESENT);

  BLECharacteristic *pCharCPSFeature = pService->createCharacteristic(NimBLEUUID(CYCLING_POWER_FEATURE_CHAR_UUID),

                                                                      NIMBLE_PROPERTY::READ);

  pCharCPSFeature->setValue(CPF_CRANK_REVOLUTION_DATA_SUPPORTED | CPF_WHEEL_REVOLUTION_DATA_SUPPORTED);

  BLECharacteristic *pCharControl_point = pService->createCharacteristic(

      "2A66", NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::INDICATE);

  pCharControl_point->setCallbacks(new MyCallback());

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

int random(int min, int max) // range : [min, max]

{

  static bool first = true;

  if (first)

  {

    srand(time(NULL)); // seeding for the first time only!

    first = false;
  }

  return min + rand() % ((max + 1) - min);
}

void BLECyclingPowerService::loop_BLE_server_multiconnect_NimBLE(uint16_t currentPower, Cadence &cadence)

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

    CPSMeasurement.flags = (CPM_WHEEL_REV_DATA_PRESENT | CPM_CRANK_REV_DATA_PRESENT);

    CPSMeasurement.power = currentPower;

    CPSMeasurement.wheel_revs = (cadence.totalRevs() * 31);

    CPSMeasurement.wheel_rev_timestamp = (cadence.getGattLastCrankRevolutionTimestamp() * 2) % 65536;

    CPSMeasurement.crank_revs = cadence.totalRevs();

    CPSMeasurement.crank_rev_timestamp = cadence.getGattLastCrankRevolutionTimestamp();

    // Serial.println(freq.getGattLastCrankRevolutionTimestamp());

    pCharacteristic->setValue(reinterpret_cast<uint8_t *>(&CPSMeasurement), sizeof(CPSMeasurement));

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