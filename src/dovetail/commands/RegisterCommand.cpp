//
// Created by Ezra Golombek on 11/06/2026.
//

#include "RegisterCommand.h"

#include "devices/DeviceManager.h"
#include "dovetail/DovetailSystem.h"
#include "dovetail/WifiModule.h"
#include "logging/Logger.h"
#include "store/Store.h"

String RegisterCommand::name() {
    return "register";
}

void RegisterCommand::execute(const uint8_t &wsClientID, JsonDocument doc) {
    const auto mac = String(doc["mac"]);
    Logger::log("Mac attempted to register: " + mac);
    DovetailSystem::verifyDevice(WifiModule::parsePrettyMac(mac), wsClientID);
}
