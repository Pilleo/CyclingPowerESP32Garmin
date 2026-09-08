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
#include "CpsControlPoint.h"

class BLECyclingPowerService : public IBleService, public IBleStackAdapter::Callbacks,
                               public IBleStackAdapter::CharacteristicCallbacks
{
private:
    IBleStackAdapter &_bleStack;

    // Characteristic handles
    IBleStackAdapter::CharHandle _powerMeasurementCharacteristic = nullptr;
    IBleStackAdapter::CharHandle _featureCharacteristic = nullptr;
    IBleStackAdapter::CharHandle _sensorLocationCharacteristic = nullptr;
    IBleStackAdapter::CharHandle _cpsControlPointCharacteristic = nullptr;
    IBleStackAdapter::CharHandle _cscMeasurementCharacteristic = nullptr;
    IBleStackAdapter::CharHandle _cscFeatureCharacteristic = nullptr;
    IBleStackAdapter::CharHandle _cscSensorLocationCharacteristic = nullptr;
    IBleStackAdapter::CharHandle _cscControlPointCharacteristic = nullptr;
    IBleStackAdapter::CharHandle _batteryLevelCharacteristic = nullptr;

    bool _deviceConnected = false;
    uint32_t _lastBaseCscWheelRevolutions = 0;
    uint32_t _lastBaseCpsWheelRevolutions = 0;
    int64_t _cscWheelRevolutionOffset = 0;
    CpsControlPoint _cpsControlPoint;

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

    void updateData(const CyclingTelemetry& telemetry) override;

    auto isConnected() -> bool override;

    // IBleStackAdapter::Callbacks implementation
    void onConnect() override;

    void onDisconnect() override;
    void onWrite(void *handle, const uint8_t *data, size_t length) override;
};

#endif // BLEPOWERSERVICE_H
