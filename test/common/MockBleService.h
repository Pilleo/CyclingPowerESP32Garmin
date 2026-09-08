#ifndef MOCK_BLE_SERVICE_H
#define MOCK_BLE_SERVICE_H

#include "../../src/IBleService.h"

class MockBleService : public IBleService {
public:
    uint16_t lastPowerSent = 0;
    uint32_t lastRevsSent = 0;
    uint16_t lastTimestampSent = 0;
    bool notifyCalled = false;
    bool startCalled = false;
    bool connected = false;
    int callCount = 0;

    void start() override {
        startCalled = true;
    }

    void updateData(const CyclingTelemetry& telemetry) override {
        lastPowerSent = telemetry.powerWatts;
        lastRevsSent = telemetry.crankRevolutions;
        lastTimestampSent = telemetry.crankEventTime1024;
        notifyCalled = true;
        callCount++;
    }

    bool isConnected() override {
        return connected;
    }
};

#endif // MOCK_BLE_SERVICE_H
