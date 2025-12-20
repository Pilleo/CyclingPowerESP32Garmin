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

    void updateData(uint16_t power, uint32_t revs, uint16_t timestamp) override {
        lastPowerSent = power;
        lastRevsSent = revs;
        lastTimestampSent = timestamp;
        notifyCalled = true;
        callCount++;
    }

    bool isConnected() override {
        return connected;
    }
};

#endif // MOCK_BLE_SERVICE_H
