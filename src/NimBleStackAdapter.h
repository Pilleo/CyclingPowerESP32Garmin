#ifndef NIMBLESTACKADAPTER_H
#define NIMBLESTACKADAPTER_H

#ifndef NATIVE_TEST
#include "IBleStackAdapter.h"
#include <NimBLEDevice.h>
#include <map>
#include <string>

class NimBleStackAdapter : public IBleStackAdapter {
    class ServerCallbacks : public BLEServerCallbacks {
        NimBleStackAdapter& _adapter;
    public:
        ServerCallbacks(NimBleStackAdapter& adapter) : _adapter(adapter) {}
        void onConnect(BLEServer* pServer, NimBLEConnInfo& connInfo) override {
            if (_adapter._callbacks) _adapter._callbacks->onConnect();
        }
        void onDisconnect(BLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
            if (_adapter._callbacks) _adapter._callbacks->onDisconnect();
        }
    };

    BLEServer* _pServer = nullptr;
    Callbacks* _callbacks = nullptr;
    ServerCallbacks* _serverCallbacks = nullptr;

    // Map to store services and characteristics
    // Simple mapping: UUID string -> BLEService*
    std::map<std::string, BLEService*> _services;

    // We need to map our opaque handle back to BLECharacteristic*
    // Since we return void*, we can just cast BLECharacteristic* to void* and back.

public:
    NimBleStackAdapter() = default;

    void init(const char* deviceName) override {
        NimBLEDevice::init(deviceName);
        _pServer = BLEDevice::createServer();
        _serverCallbacks = new ServerCallbacks(*this);
        _pServer->setCallbacks(_serverCallbacks);
    }

    void startAdvertising() override {
        BLEDevice::startAdvertising();
    }

    void setCallbacks(Callbacks* callbacks) override {
        _callbacks = callbacks;
    }

    void createService(const char* uuid) override {
        BLEService* pService = _pServer->createService(NimBLEUUID(uuid));
        _services[uuid] = pService;
    }

    void startService(const char* uuid) override {
        if (_services.find(uuid) != _services.end()) {
            _services[uuid]->start();
        }
    }

    CharHandle createCharacteristic(const char* serviceUuid, const char* charUuid, uint32_t properties) override {
        if (_services.find(serviceUuid) == _services.end()) return nullptr;

        // Map properties. Assuming simple mapping for now.
        // NimBLE properties are bitmasks.
        uint32_t nimProps = 0;
        if (properties & PROP_READ) nimProps |= NIMBLE_PROPERTY::READ;
        if (properties & PROP_NOTIFY) nimProps |= NIMBLE_PROPERTY::NOTIFY;

        BLECharacteristic* pChar = _services[serviceUuid]->createCharacteristic(NimBLEUUID(charUuid), nimProps);
        return static_cast<CharHandle>(pChar);
    }

    void setCharacteristicValue(CharHandle handle, const uint8_t* data, size_t length) override {
        if (!handle) return;
        static_cast<BLECharacteristic*>(handle)->setValue(data, length);
    }

    void setCharacteristicValue(CharHandle handle, uint8_t value) override {
        if (!handle) return;
        static_cast<BLECharacteristic*>(handle)->setValue(value);
    }

    void notify(CharHandle handle) override {
        if (!handle) return;
        static_cast<BLECharacteristic*>(handle)->notify();
    }

    void addServiceToAdvertising(const char* uuid) override {
        BLEDevice::getAdvertising()->addServiceUUID(NimBLEUUID(uuid));
    }

    void setAppearance(uint16_t appearance) override {
        BLEDevice::getAdvertising()->setAppearance(appearance);
    }
};
#endif

#endif // NIMBLESTACKADAPTER_H
