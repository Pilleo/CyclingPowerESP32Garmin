#ifndef BIKECOMPUTER_H
#define BIKECOMPUTER_H

#include <cstdint>
#include "SystemWrapper.h"
#include "Cadence.h"
#include "ResistanceLevel.h"
#include "IBleService.h"
#include "Power.h"
#include "StatusLed.h"

struct BikeComputerConfig {
    uint8_t ledPin;
    uint8_t wakeupPin;
    uint8_t cadencePin;
    uint8_t resPositionPin; //This pin is blinking a lot when resistance is changing
    uint8_t resMinLevelLimitPin; //active when resistance is in min state
    uint8_t resBackwardsDirectionIndicatorPin; // supposed to change state rare
    uint8_t resForwardDirectionIndicatorPin; // supposed to change state rare
};

class BikeComputer {
    ISystemWrapper &_sys;
    Cadence &_cadence;
    ResistanceLevel &_resistance;
    IBleService &_bleService;
    BikeComputerConfig _config;
    StatusLed _led;

    unsigned long lastDataSentTimestamp = 0;
    uint16_t currentPower = 0;
    uint8_t lastLevel = 1;

    // Constants
    static constexpr int timePerioudForSendingData = 510;

public:
    BikeComputer(ISystemWrapper &sys, Cadence &cad, ResistanceLevel &res, IBleService &ble,
                 BikeComputerConfig config = {27, 15}) noexcept
        : _sys(sys), _cadence(cad), _resistance(res), _bleService(ble), _config(config), _led(sys, config.ledPin) {
    }

    void setup() {
        _cadence.begin();
        _resistance.begin();
        _led.setup();
        _bleService.start();
        //   setupInterrupts();
    }

    void setupInterrupts() const {
        _cadence.enableInterrupt();
        _resistance.enableInterrupt();

        // Attach interrupts using the config
        _sys.attachInterrupt(_config.cadencePin, Cadence::isr, FALLING);
        _sys.attachInterrupt(_config.resPositionPin, ResistanceLevel::isrPosition, CHANGE);
        _sys.attachInterrupt(_config.resMinLevelLimitPin, ResistanceLevel::isrLimit, RISING);

        // Note: resBackwardsPin and resForwardPin are not attached here,
        // but they are now documented in the config object.
    }

    void update() {
        // POLL SENSORS (New Step)
        // When you switch to Interrupts, you will just comment these two lines out!
        _cadence.poll();
        _resistance.poll();

        const int16_t cad = _cadence.cadence();

        _led.update(cad);

        const unsigned long ms = _sys.millis();
        const uint8_t l = _resistance.level();
        if (lastLevel != l) {
#ifndef NATIVE_TEST
            // Print Level and the raw Counter value to help debug
            Serial.print("RESISTANCE CHANGE: Level ");
            Serial.print(l);
            Serial.print(" (Raw Counter: ");
            Serial.print(_resistance.getPositionChangeCounter());
            Serial.println(")");
#endif
            lastLevel = l;
        }

        currentPower = Power::calculate({l, static_cast<float>(cad)});

        const unsigned long periodSinceLastTransaction = ms - lastDataSentTimestamp;

        if (periodSinceLastTransaction >= timePerioudForSendingData) {
            if (cad >= 0) {
                _bleService.updateData(currentPower, _cadence.totalRevs(),
                                       _cadence.getGattLastCrankRevolutionTimestamp());
                lastDataSentTimestamp = ms;
            } else {
                const bool cadenceState = _sys.digitalRead(_config.wakeupPin) != 0;
                _sys.enterDeepSleep(_config.wakeupPin, static_cast<int>(!cadenceState));
            }
        }
    }
};

#endif //BIKECOMPUTER_H
