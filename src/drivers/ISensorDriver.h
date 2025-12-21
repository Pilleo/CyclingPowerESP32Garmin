#ifndef ISENSORDRIVER_H
#define ISENSORDRIVER_H

#include <cstdint>

class ISensorDriver {
public:
    virtual ~ISensorDriver() = default;

    virtual void begin() = 0;
    virtual void update() = 0;
    virtual uint32_t getEventCount() const = 0;
    virtual uint32_t getLastEventTime() const = 0;
};

#endif // ISENSORDRIVER_H
