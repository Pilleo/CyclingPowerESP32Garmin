//
// Created by leanid on 26.1.24.
//

#ifndef BLEPOWERSERVICE_H
#define BLEPOWERSERVICE_H
#include <Arduino.h>



class BLECyclingPowerService {
public:
    static void loop_BLE_server_multiconnect_NimBLE(uint16_t  currentPower);
    static void setup_BLE_server_multiconnect_NimBLE();
};



#endif //BLEPOWERSERVICE_H
