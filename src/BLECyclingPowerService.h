//
// Created by leanid on 26.1.24.
//

#ifndef BLEPOWERSERVICE_H
#define BLEPOWERSERVICE_H

#ifndef NATIVE_TEST
#include <Arduino.h>
#endif
#include "Cadence.h"
#include "IBleService.h"
#include "IBleStackAdapter.h"

class BLECyclingPowerService : public IBleService, public IBleStackAdapter::Callbacks {
    IBleStackAdapter& _bleStack;

    IBleStackAdapter::CharHandle _hPowerMeas = nullptr;
    IBleStackAdapter::CharHandle _hFeature = nullptr;
    IBleStackAdapter::CharHandle _hSensorLoc = nullptr;
    IBleStackAdapter::CharHandle _hBattery = nullptr;

    bool _deviceConnected = false;
    bool _oldDeviceConnected = false;

public:
    explicit BLECyclingPowerService(IBleStackAdapter& bleStack);

    void start() override;
    void updateData(uint16_t power, uint32_t revs, uint16_t timestamp) override;
    bool isConnected() override;

    // IBleStackAdapter::Callbacks implementation
    void onConnect() override;
    void onDisconnect() override;

    // Deprecated static methods removed or kept as wrappers if absolutely necessary
    // For now, removing them to enforce new architecture.
};

#endif //BLEPOWERSERVICE_H
