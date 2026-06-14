//
// Created by Ezra Golombek on 14/06/2026.
//

#ifndef PHISILANDDISPLAY_DEVICEMANAGER_H
#define PHISILANDDISPLAY_DEVICEMANAGER_H
#include <IPAddress.h>
#include <map>
#include <ArduinoJson.h>
#include "ClientId.h"

/**
 * DevicesManager handles caching information about connected clients, what code is assigned to each device and more.
 * A device's ID is it's MAC address, it can then be given a nickname for programming.
 */
class DeviceManager {
    std::map<String, ClientId> nameToMac;
    std::map<ClientId, String> macToName;
    std::map<ClientId, String> macToCode;
    std::map<ClientId, u_int32_t> registeredDeviceMacToClientId; // Mac -> WS Client (Make's sure that we dont end up having fake entries we use mac as key.)

    DeviceManager() {
        nameToMac = {};
        macToName = {};
        macToCode = {};
        registeredDeviceMacToClientId = {};
    }

    ~DeviceManager() {
    }

public:
    DeviceManager(const DeviceManager &) = delete;

    DeviceManager &operator=(const DeviceManager &) = delete;

    static DeviceManager &getInstance() {
        static DeviceManager instance;
        return instance;
    }

    void clearDeviceCache();

    JsonDocument serializeDevicesToJson();

    void loadDevicesToCache(JsonDocument doc);

    void assignCodeBaseToDevice(ClientId id, const String &codebase);

    void addDevice(ClientId mac, const String &name);

    void addDevice(ClientId mac, const String &name, const String &codebase);

    String getCodeBaseForId(const String &id);

    String getScriptFilePathByMac(const ClientId &mac);

    bool canJoinNetwork(ClientId id);

    bool isRegistered(ClientId id) const;

    ClientId getDeviceIdByName(const String &name);

    String getDeviceNameById(ClientId id);

    void renameDevice(const String &oldName, const String &newName);

    void renameDevice(ClientId id, const String &newName);

    std::map<ClientId, u_int32_t> getConnectedDevices();

    u_int32_t getWSClientByMac(ClientId id);

    void registerDevice(ClientId id, uint32_t wsClientId);

};


#endif //PHISILANDDISPLAY_DEVICEMANAGER_H
