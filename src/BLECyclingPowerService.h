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


class BLECyclingPowerService : public IBleService {
public:
    void start() override;
    void updateData(uint16_t power, uint32_t revs, uint16_t timestamp) override;
    bool isConnected() override;

    // Keep static for backward compatibility if needed, or refactor completely
    static void loop_BLE_server_multiconnect_NimBLE( uint16_t  currentPower, const Cadence &cadence);
    static void setup_BLE_server_multiconnect_NimBLE();
};



#endif //BLEPOWERSERVICE_H
