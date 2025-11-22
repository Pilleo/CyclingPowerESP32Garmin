class ResistanceLevel
{
public:
    explicit ResistanceLevel(uint8_t backwardsPin, uint8_t limitPin, uint8_t positionPin, uint8_t forwardsPin);

    uint8_t level();

private:
    const uint8_t limittPin;
    const uint8_t backwardsPin;
    const uint8_t positionPin;
    const uint8_t forwardsPin;
    bool oldState;
    uint8_t currentLevel;
    uint32_t elapsedSampleTimeForLevel;
    bool prevDirection;
    uint16_t positionChangeCounter;

    void isFirstLevel(bool back);
    bool wasInPositiveDirection() const;
    static bool isConsistentMovement(unsigned long sampleTime);
    static bool isMovementAfterLongPause(unsigned long sampleTime);
};
