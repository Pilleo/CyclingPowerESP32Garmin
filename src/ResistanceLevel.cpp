#include <Arduino.h>
#include <ResistanceLevel.h>

bool oldStatee;
unsigned long elapsedTimee = 0;
unsigned long elapsedSampleTimee = 0;

uint8_t currentLevel = 1;
uint32_t elapsedSampleTimeForLevel = 0;
bool prevDirection = true;

uint16_t positionChangeCounter, prevPositionChangeCounter = 0;

ResistanceLevel::ResistanceLevel(const uint8_t backwardsPin, const uint8_t limittPin, const uint8_t positionPin,
                                 const uint8_t forwardsPin) : backwardsPin(backwardsPin), limittPin(limittPin),
                                                              positionPin(positionPin), forwardsPin(forwardsPin) {
    pinMode(INPUT_PULLUP, backwardsPin);
    pinMode(INPUT_PULLUP, forwardsPin);
    pinMode(INPUT_PULLUP, positionPin);
    pinMode(INPUT_PULLUP, limittPin);
    oldStatee = digitalRead(positionPin);
}

bool wasInPositiveDirection() {
    return prevDirection;
}

void ResistanceLevel::isFirstLevel(bool forward) const {
    const bool limitState = digitalRead(limittPin);

    if (!forward && limitState) {
        positionChangeCounter = 0;
        currentLevel = 1;
    }
}

bool isConsistentMovement(unsigned long sampleTime) {
    return sampleTime < 50;
}

bool isMovementAfterLongPause(unsigned long sampleTime) {
    return sampleTime > 1700;
}

uint8_t ResistanceLevel::level() const {
    const bool preBack = digitalRead(backwardsPin);
    const bool preForward = digitalRead(forwardsPin);
    const bool positionState = digitalRead(positionPin);

    const bool back = preBack && preForward == false;
    const bool forward = preForward && preBack == false;
    isFirstLevel(forward);

    const unsigned long mls = millis();
    const unsigned long sampleTime = mls - elapsedSampleTimeForLevel;

    if (positionState != oldStatee && sampleTime > 8) {
        oldStatee = positionState;
        elapsedSampleTimeForLevel = mls;
        prevPositionChangeCounter = positionChangeCounter;

        if (forward && ((isConsistentMovement(sampleTime) && wasInPositiveDirection()) ||
                        isMovementAfterLongPause(sampleTime))) {
            positionChangeCounter++;
        } else if (positionChangeCounter > 0 && back && (
                       (isMovementAfterLongPause(sampleTime)) || (
                           isConsistentMovement(sampleTime) && !wasInPositiveDirection()))) //
        {
            positionChangeCounter--;
        }
        prevDirection = forward;

        // Serial.print("prevPositionChangeCounter ");
        //   Serial.println(prevPositionChangeCounter);

        //  Serial.print("positionChangeCounter ");
        // Serial.println(positionChangeCounter);

        // Serial.print("sampleTime ");
        // Serial.println(sampleTime);

        // if (forward)
        // {
        //     Serial.println("forward");
        // }
        // if (back)
        // {
        //     Serial.println("back");
        // }

        if (positionChangeCounter < 30) {
            currentLevel = 1;
        } else if (positionChangeCounter > 169 && positionChangeCounter < 191) {
            currentLevel = 2;
        } else if (positionChangeCounter > 280 && positionChangeCounter < 309) {
            currentLevel = 3;
        } else if (positionChangeCounter > 359 && positionChangeCounter < 375) {
            currentLevel = 4;
        } else if (positionChangeCounter > 410 && positionChangeCounter < 430) {
            currentLevel = 5;
        } else if (positionChangeCounter > 460 && positionChangeCounter <= 475) {
            currentLevel = 6;
        } else if (positionChangeCounter > 500 && positionChangeCounter < 520) {
            currentLevel = 7;
        } else if (positionChangeCounter > 530 && positionChangeCounter < 550) {
            currentLevel = 8;
        } else if (positionChangeCounter > 560 && positionChangeCounter < 576) {
            currentLevel = 9;
        } else if (positionChangeCounter > 590 && positionChangeCounter <= 600) {
            currentLevel = 10;
        } else if (positionChangeCounter > 610 && positionChangeCounter <= 626) {
            currentLevel = 11;
        } else if (positionChangeCounter > 630 && positionChangeCounter <= 640) {
            currentLevel = 12;
        } else if (positionChangeCounter > 652 && positionChangeCounter <= 660) {
            currentLevel = 13;
        } else if (positionChangeCounter > 665 && positionChangeCounter <= 678) {
            currentLevel = 14;
        } else if (positionChangeCounter > 680 && positionChangeCounter <= 689) {
            currentLevel = 15;
        } else if (positionChangeCounter > 691) {
            currentLevel = 16;
        }
    }
    return currentLevel;
}
