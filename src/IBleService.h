#ifndef IBLESERVICE_H
#define IBLESERVICE_H

#include <cstdint>

class IBleService {
public:
    virtual ~IBleService() = default;
    virtual void start() = 0;
    virtual void updateData(uint16_t power, uint32_t revs, uint16_t timestamp) = 0;
    virtual bool isConnected() = 0;
};

#endif //IBLESERVICE_H
