//
// Created by Ezra Golombek on 14/06/2026.
//

#include "DeviceManager.h"

#include <ArduinoJson.h>

#include "ClientId.h"
#include "dovetail/DovetailSystem.h"
#include "dovetail/WifiModule.h"
#include "hoist/HoistSystem.h"
#include "logging/Logger.h"

void DeviceManager::clearDeviceCache() {
    macToCode.clear();
    nameToMac.clear();
    macToName.clear();
    //Maybe also burn all these clients ?
    registeredDeviceMacToClientId.clear();
}

JsonDocument DeviceManager::serializeDevicesToJson() {
    JsonDocument doc;

    for (auto const &[mac, code]: macToCode) {
        auto formattedMac = WifiModule::macToString(mac);

        doc[formattedMac]["code"] = code;

        doc[formattedMac]["name"] = macToName[mac];
    }
    return doc;
}

void DeviceManager::loadDevicesToCache(JsonDocument doc) {
    clearDeviceCache();
    const auto root = doc.as<JsonObject>();

    for (JsonPair deviceEntry: root) {
        String formattedMac = deviceEntry.key().c_str();
        const auto mac = WifiModule::parsePrettyMac(formattedMac);

        JsonObject data = deviceEntry.value();
        String name = data["name"] | "NoName";
        String code = data["code"] | "default.ezra";

        addDevice(mac, name, code);
    }
    Logger::log("Loaded " + String(macToCode.size()) + " devices from SD.");
}


void DeviceManager::assignCodeBaseToDevice(const ClientId id, const String &codebase) {
    macToCode[id] = codebase;
}

void DeviceManager::addDevice(const ClientId mac, const String &name) {
    nameToMac[name] = mac;
    macToName[mac] = name;
}

void DeviceManager::addDevice(const ClientId mac, const String &name, const String &codebase) {
    addDevice(mac, name);
    macToCode[mac] = codebase;
}

String DeviceManager::getCodeBaseForId(const String &id) {
    const auto mac = nameToMac[id];
    const auto codeBase = macToCode[mac];
    return codeBase;
}

String DeviceManager::getScriptFilePathByMac(const ClientId &mac) {
    auto assignedScriptToMac = macToCode.find(mac);
    auto macHasAssignedCodebase = assignedScriptToMac != macToCode.end();
    if (HoistSystem::getInstance().isHoisting())
        return String("/scripts/") + HoistSystem::getInstance().assignedFileForNewDevice();
    return String("/scripts/") + (macHasAssignedCodebase ? assignedScriptToMac->second : "default.ezra");
}

void DeviceManager::onDeviceConnect() {
    HoistSystem::getInstance().onDeviceConnect();
}


bool DeviceManager::onDeviceRegistration(const ClientId id) {
    if (macToCode.find(id) != macToCode.end())
        return true;
    if (HoistSystem::getInstance().isHoisting())
        return true;
    //We may need to add another function to verify the entire process rather then just check if they are allowed.
    if (DovetailSystem::connectMode)
        return true;
    return false;
}

bool DeviceManager::isRegistered(const ClientId id) const {
    return registeredDeviceMacToClientId.count(id);
}

ClientId DeviceManager::getDeviceIdByName(const String &name) {
    return nameToMac[name];
}

String DeviceManager::getDeviceNameById(const ClientId id) {
    return macToName[id];
}

void DeviceManager::renameDevice(const String &oldName, const String &newName) {
    const auto id = nameToMac[oldName];
    nameToMac.erase(oldName);
    macToName[id] = newName;
    nameToMac[newName] = id;
}

void DeviceManager::renameDevice(const ClientId id, const String &newName) {
    const auto oldName = macToName[id];
    nameToMac.erase(oldName);
    macToName[id] = newName;
    nameToMac[newName] = id;
}


const std::map<ClientId, u_int32_t> &DeviceManager::getConnectedDevices() {
    return registeredDeviceMacToClientId;
}

u_int32_t DeviceManager::getWSClientByMac(const ClientId id) {
    return registeredDeviceMacToClientId[id];
}

void DeviceManager::registerDevice(const ClientId id, uint32_t wsClientId) {
    Logger::log((WifiModule::macToString(id) + " is now registered!").data());
    registeredDeviceMacToClientId[id] = wsClientId;
}
