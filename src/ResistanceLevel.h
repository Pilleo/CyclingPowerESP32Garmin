class ResistanceLevel {
public:
    explicit ResistanceLevel(uint8_t backwardsPin, uint8_t limitPin, uint8_t positionPin, uint8_t forwardsPin);

    uint8_t level() const;

private:
    const uint8_t limittPin;
    const uint8_t backwardsPin;
    const uint8_t positionPin;
    const uint8_t forwardsPin;

    void isFirstLevel(bool back) const;
};
