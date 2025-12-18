//
// Created by leanid on 26.1.24.
//

#ifndef BLEPOWERSERVICE_H
#define BLEPOWERSERVICE_H

#ifndef NATIVE_TEST
#include <Arduino.h>
#endif
#include "Cadence.h"



class BLECyclingPowerService {
public:
    static void loop_BLE_server_multiconnect_NimBLE( uint16_t  currentPower, const Cadence &cadence);
    static void setup_BLE_server_multiconnect_NimBLE();
};



#endif //BLEPOWERSERVICE_H
