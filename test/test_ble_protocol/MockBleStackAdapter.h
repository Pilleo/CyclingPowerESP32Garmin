#ifndef MOCK_BLE_STACK_ADAPTER_H
#define MOCK_BLE_STACK_ADAPTER_H

#include "../../src/IBleStackAdapter.h"
#include <string>
#include <map>
#include <vector>

class MockBleStackAdapter : public IBleStackAdapter {
public:
    struct CharacteristicData {
        std::string serviceUuid;
        std::string charUuid;
        uint32_t properties;
        std::vector<uint8_t> value;
        bool notified;
    };

    Callbacks* callbacks = nullptr;
    bool advertisingStarted = false;
    uint16_t appearance = 0;
    std::map<CharHandle, CharacteristicData> characteristics;
    std::vector<std::string> createdServices;
    std::vector<std::string> startedServices;
    std::vector<std::string> advertisedServices;

    // Helper to generate handles
    uintptr_t nextHandle = 1;

    void init(const char* deviceName) override {}

    void startAdvertising() override {
        advertisingStarted = true;
    }

    void setCallbacks(Callbacks* cb) override {
        callbacks = cb;
    }

    void createService(const char* uuid) override {
        createdServices.push_back(uuid);
    }

    void startService(const char* uuid) override {
        startedServices.push_back(uuid);
    }

    CharHandle createCharacteristic(const char* serviceUuid, const char* charUuid, uint32_t properties) override {
        CharHandle handle = reinterpret_cast<CharHandle>(nextHandle++);
        characteristics[handle] = {serviceUuid, charUuid, properties, {}, false};
        return handle;
    }

    void setCharacteristicValue(CharHandle handle, const uint8_t* data, size_t length) override {
        if (characteristics.find(handle) != characteristics.end()) {
            characteristics[handle].value.assign(data, data + length);
        }
    }

    void setCharacteristicValue(CharHandle handle, uint8_t value) override {
        if (characteristics.find(handle) != characteristics.end()) {
            characteristics[handle].value = {value};
        }
    }

    void notify(CharHandle handle) override {
        if (characteristics.find(handle) != characteristics.end()) {
            characteristics[handle].notified = true;
        }
    }

    void addServiceToAdvertising(const char* uuid) override {
        advertisedServices.push_back(uuid);
    }
    void setAppearance(uint16_t value) override {
        appearance = value;
    }

    // Test Helpers
    void simulateConnect() {
        if (callbacks) callbacks->onConnect();
    }

    void simulateDisconnect() {
        if (callbacks) callbacks->onDisconnect();
    }

    bool wasNotified(const char* charUuid) {
        for (const auto& pair : characteristics) {
            if (pair.second.charUuid == charUuid && pair.second.notified) {
                return true;
            }
        }
        return false;
    }

    std::vector<uint8_t> getValue(const char* charUuid) {
        for (const auto& pair : characteristics) {
            if (pair.second.charUuid == charUuid) {
                return pair.second.value;
            }
        }
        return {};
    }

    const CharacteristicData* findCharacteristic(const char* serviceUuid,
                                                  const char* charUuid) const {
        for (const auto& pair : characteristics) {
            if (pair.second.serviceUuid == serviceUuid &&
                pair.second.charUuid == charUuid) {
                return &pair.second;
            }
        }
        return nullptr;
    }

    void clearNotifications() {
        for (auto& pair : characteristics) {
            pair.second.notified = false;
        }
    }
};

#endif // MOCK_BLE_STACK_ADAPTER_H
