#ifndef BIKECOMPUTER_H
#define BIKECOMPUTER_H

#include "SystemWrapper.h"
#include "Cadence.h"
#include "ResistanceLevel.h"
#include "IBleService.h"
#include "Power.h"
#include "StatusLed.h"

struct BikeComputerConfig {
    uint8_t ledPin;
    uint8_t wakeupPin;
};

class BikeComputer {
    ISystemWrapper& _sys;
    Cadence& _cadence;
    ResistanceLevel& _resistance;
    IBleService& _bleService;
    BikeComputerConfig _config;
    StatusLed _led;

    unsigned long lastDataSentTimestamp = 0;
    uint16_t currentPower = 0;
    uint8_t lastLevel = 1;

    // Constants
    static constexpr int timePerioudForSendingData = 510;

public:
    BikeComputer(ISystemWrapper& sys, Cadence& cad, ResistanceLevel& res, IBleService& ble, BikeComputerConfig config = {27, 15})
        : _sys(sys), _cadence(cad), _resistance(res), _bleService(ble), _config(config), _led(sys, config.ledPin) {}

    void setup() {
        _led.setup();
        _bleService.start();
    }

    void update() {
        const int16_t cad = _cadence.cadence();

        _led.update(cad);

        const unsigned long ms = _sys.millis();
        const uint8_t l = _resistance.level();
        if (lastLevel != l)
        {
            lastLevel = l;
        }

        currentPower = Power::power(l, cad);

        const unsigned long periodSinceLastTransaction = ms - lastDataSentTimestamp;

        if (periodSinceLastTransaction >= timePerioudForSendingData)
        {
            if (cad >= 0)
            {
                _bleService.updateData(currentPower, _cadence.totalRevs(), _cadence.getGattLastCrankRevolutionTimestamp());
                lastDataSentTimestamp = ms;
            }
            else
            {
                const bool cadenceState = _sys.digitalRead(_config.wakeupPin) != 0;
                _sys.enterDeepSleep(_config.wakeupPin, static_cast<int>(!cadenceState));
            }
        }
    }
};

#endif //BIKECOMPUTER_H
