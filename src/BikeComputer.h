#ifndef BIKECOMPUTER_H
#define BIKECOMPUTER_H

#include "SystemWrapper.h"
#include "Cadence.h"
#include "ResistanceLevel.h"
#include "IBleService.h"
#include "Power.h"

class BikeComputer {
    ISystemWrapper& _sys;
    Cadence& _cadence;
    ResistanceLevel& _resistance;
    IBleService& _bleService;

    unsigned long lastDataSentTimestamp = 0;
    uint16_t currentPower = 0;
    uint8_t lastLevel = 1;
    uint32_t lastBlink = 0;
    int incomingRPM = 45;

    // Constants
    static constexpr int timePerioudForSendingData = 510;
    static constexpr uint8_t ledPin = 27;
    static constexpr uint8_t wakeupPin = 15; // GPIO_NUM_15

public:
    BikeComputer(ISystemWrapper& sys, Cadence& cad, ResistanceLevel& res, IBleService& ble)
        : _sys(sys), _cadence(cad), _resistance(res), _bleService(ble) {}

    void setup() {
        _sys.digitalWrite(ledPin, 0); // Initialize LED state if needed, though pinMode is usually setup
        // Note: pinMode is typically Arduino specific and might need abstraction if strictly testing logic
        // For now, assuming setup() in main handles hardware init like Serial and pinMode
        _bleService.setup();
    }

    void update() {
        int const timeFrameForBlink = 60000 / incomingRPM;
        long const start = _sys.millis();

        if (start - lastBlink >= timeFrameForBlink)
        {
            _sys.digitalWrite(ledPin, 0); // LOW
            lastBlink = start;
        }
        else if (start - lastBlink >= 1)
        {
            _sys.digitalWrite(ledPin, 1); // HIGH
        }

        const unsigned long ms = _sys.millis();
        const uint8_t l = _resistance.level();
        if (lastLevel != l)
        {
            // Serial.print("Level:         ");
            // Serial.println(l);
            lastLevel = l;
        }

        const int16_t cad = _cadence.cadence();
        currentPower = Power::power(l, cad);

        const unsigned long periodSinceLastTransaction = ms - lastDataSentTimestamp;

        if (periodSinceLastTransaction >= timePerioudForSendingData)
        {
            if (cad >= 0)
            {
                _bleService.update(currentPower, _cadence);
                lastDataSentTimestamp = ms;
            }
            else
            {
                // Serial.println("nothing to sent, rpm is -1");
                const bool cadenceState = _sys.digitalRead(wakeupPin) != 0;
                _sys.enterDeepSleep(wakeupPin, static_cast<int>(!cadenceState));
            }
        }
    }
};

#endif //BIKECOMPUTER_H
