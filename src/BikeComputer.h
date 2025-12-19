#ifndef BIKECOMPUTER_H
#define BIKECOMPUTER_H

#include "SystemWrapper.h"
#include "Cadence.h"
#include "ResistanceLevel.h"
#include "IBleService.h"
#include "Power.h"

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

    unsigned long lastDataSentTimestamp = 0;
    uint16_t currentPower = 0;
    uint8_t lastLevel = 1;
    uint32_t lastBlink = 0;

    // Removed incomingRPM as it was constant and unused effectively
    // static constexpr int incomingRPM = 45;

    // Constants
    static constexpr int timePerioudForSendingData = 510;

public:
    BikeComputer(ISystemWrapper& sys, Cadence& cad, ResistanceLevel& res, IBleService& ble, BikeComputerConfig config = {27, 15})
        : _sys(sys), _cadence(cad), _resistance(res), _bleService(ble), _config(config) {}

    void setup() {
        _sys.digitalWrite(_config.ledPin, 0);
        _bleService.start();
    }

    void update() {
        const int16_t cad = _cadence.cadence();

        // Blink Logic based on ACTUAL cadence
        // If cadence is 0, maybe don't blink or blink slowly?
        // Original logic: 60000 / incomingRPM.
        // New logic: 60000 / cad (if cad > 0)

        int timeFrameForBlink = 1000; // Default slow blink if 0 rpm
        if (cad > 0) {
            timeFrameForBlink = 60000 / cad;
        }

        long const start = _sys.millis();

        if (start - lastBlink >= timeFrameForBlink)
        {
            _sys.digitalWrite(_config.ledPin, 0); // LOW
            lastBlink = start;
        }
        else if (start - lastBlink >= 1)
        {
            _sys.digitalWrite(_config.ledPin, 1); // HIGH
        }

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
