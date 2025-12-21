#ifndef IBLESTACKADAPTER_H
#define IBLESTACKADAPTER_H

#include <cstdint>
#include <cstddef>

class IBleStackAdapter {
public:
    struct Callbacks {
        virtual void onConnect() = 0;
        virtual void onDisconnect() = 0;
    };

    // Properties abstraction
    static constexpr uint32_t PROP_READ = 0x01;
    static constexpr uint32_t PROP_NOTIFY = 0x10; // Matching typical BLE standard values roughly, or just mapping

    virtual ~IBleStackAdapter() = default;

    virtual void init(const char* deviceName) = 0;
    virtual void startAdvertising() = 0;
    virtual void setCallbacks(Callbacks* callbacks) = 0;

    // Abstract Characteristic Handle
    using CharHandle = void*;

    virtual void createService(const char* uuid) = 0;
    virtual void startService(const char* uuid) = 0;

    virtual auto createCharacteristic(const char* serviceUuid, const char* charUuid, uint32_t properties) -> CharHandle = 0;

    virtual void setCharacteristicValue(CharHandle handle, const uint8_t* data, size_t length) = 0;
    virtual void setCharacteristicValue(CharHandle handle, uint8_t value) = 0; // Overload for byte
    virtual void notify(CharHandle handle) = 0;

    // For advertising setup
    virtual void addServiceToAdvertising(const char* uuid) = 0;
    virtual void setAppearance(uint16_t appearance) = 0;
};

#endif // IBLESTACKADAPTER_H
