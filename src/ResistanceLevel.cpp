#include "ResistanceLevel.h"

ResistanceLevel::ResistanceLevel(const uint8_t backwardsPin, const uint8_t limittPin, const uint8_t positionPin,
                                 const uint8_t forwardsPin, ISystemWrapper& sys) : backwardsPin(backwardsPin), limittPin(limittPin),
                                                              positionPin(positionPin), forwardsPin(forwardsPin), sys(sys) {
    // pinMode(INPUT_PULLUP, backwardsPin); // This should be handled by the system wrapper
    // pinMode(INPUT_PULLUP, forwardsPin);
    // pinMode(INPUT_PULLUP, positionPin);
    // pinMode(INPUT_PULLUP, limittPin);
    oldState = sys.digitalRead(positionPin);
    currentLevel = 1;
    elapsedSampleTimeForLevel = 0;
    prevDirection = true;
    positionChangeCounter = 0;
}

bool ResistanceLevel::wasInPositiveDirection() const {
    return prevDirection;
}

void ResistanceLevel::isFirstLevel(bool forward) {
    const bool limitState = sys.digitalRead(limittPin);

    if (!forward && limitState) {
        positionChangeCounter = 0;
        currentLevel = 1;
    }
}

bool ResistanceLevel::isConsistentMovement(unsigned long sampleTime) {
    return sampleTime < 50;
}

bool ResistanceLevel::isMovementAfterLongPause(unsigned long sampleTime) {
    return sampleTime > 1700;
}

uint8_t ResistanceLevel::level() {
    const bool preBack = sys.digitalRead(backwardsPin);
    const bool preForward = sys.digitalRead(forwardsPin);
    const bool positionState = sys.digitalRead(positionPin);

    const bool back = preBack && preForward == false;
    const bool forward = preForward && preBack == false;
    isFirstLevel(forward);

    const unsigned long mls = sys.millis();
    const unsigned long sampleTime = mls - elapsedSampleTimeForLevel;

    if (positionState != oldState && sampleTime > 8) {
        oldState = positionState;
        elapsedSampleTimeForLevel = mls;

        if (forward && ((isConsistentMovement(sampleTime) && wasInPositiveDirection()) ||
                        isMovementAfterLongPause(sampleTime))) {
            positionChangeCounter++;
        } else if (positionChangeCounter > 0 && back && (
                       (isMovementAfterLongPause(sampleTime)) || (
                           isConsistentMovement(sampleTime) && !wasInPositiveDirection())))
        {
            positionChangeCounter--;
        }
        prevDirection = forward;

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
