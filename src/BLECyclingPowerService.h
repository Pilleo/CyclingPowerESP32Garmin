//
// Created by leanid on 26.1.24.
//

#ifndef BLEPOWERSERVICE_H
#define BLEPOWERSERVICE_H

#ifndef NATIVE_TEST
#include <Arduino.h>
#endif
#include "IBleService.h"
#include "IBleStackAdapter.h"

class BLECyclingPowerService : public IBleService, public IBleStackAdapter::Callbacks {
private:
    IBleStackAdapter &_bleStack;

    // Characteristic handles
    IBleStackAdapter::CharHandle _powerMeasurementCharacteristic = nullptr;
    IBleStackAdapter::CharHandle _featureCharacteristic = nullptr;
    IBleStackAdapter::CharHandle _sensorLocationCharacteristic = nullptr;
    IBleStackAdapter::CharHandle _cscMeasurementCharacteristic = nullptr;
    IBleStackAdapter::CharHandle _cscFeatureCharacteristic = nullptr;
    IBleStackAdapter::CharHandle _cscSensorLocationCharacteristic = nullptr;
    IBleStackAdapter::CharHandle _batteryLevelCharacteristic = nullptr;

    bool _deviceConnected = false;

    // Private helper methods for setup
    void setupPowerService();
    void setupCscService();
    void setupBatteryService();

    void setupAdvertising() const;

public:
    explicit BLECyclingPowerService(IBleStackAdapter &bleStack) noexcept;

    ~BLECyclingPowerService() override = default;

    BLECyclingPowerService(const BLECyclingPowerService &) = delete;

    auto operator=(const BLECyclingPowerService &) -> BLECyclingPowerService & = delete;

    BLECyclingPowerService(BLECyclingPowerService &&) = delete;

    auto operator=(BLECyclingPowerService &&) -> BLECyclingPowerService & = delete;

    void start() override;

    void updateData(uint16_t power, uint32_t totalRevolutions, uint16_t crankEventTime) override;

    auto isConnected() -> bool override;

    // IBleStackAdapter::Callbacks implementation
    void onConnect() override;

    void onDisconnect() override;
};

#endif //BLEPOWERSERVICE_H
