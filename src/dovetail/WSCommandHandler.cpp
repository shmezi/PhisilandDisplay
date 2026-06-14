//
// Created by Ezra Golombek on 09/06/2026.
//

#include "WSCommandHandler.h"

#include <map>
#include <memory>

#include "AsyncWebSocket.h"
#include "DovetailSystem.h"
#include "commands/Command.h"
#include "commands/EndActivityCommand.h"
#include "commands/OnResultCommand.h"
#include "commands/RegisterCommand.h"
#include "commands/RemoteLogCommand.h"
#include "commands/SwitchScreenCommand.h"
#include "devices/DeviceManager.h"
#include "logging/Logger.h"
#include "store/Store.h"
std::map<String, std::unique_ptr<Command> > WSCommandHandler::commands;


void WSCommandHandler::onIncomingMessage(const uint8_t &wsClientId, JsonDocument &doc) {
    const char *cmdName = doc["command"];
    if (!cmdName) {
        Logger::warn("No command field in JSON!");
        return;
    }
    const auto command = commands.find(cmdName);

    if (command == commands.end()) {
        Logger::warn("Command not found!");
        return;
    }
    command->second->execute(wsClientId, doc);
}

void WSCommandHandler::registerCommand(std::unique_ptr<Command> command) {
    const String name = command->name();
    commands[name] = std::move(command);
}


void WSCommandHandler::startEvent(const String &client, const String &eventId, int param) {
    std::string c_eventId = {eventId.c_str()};
    const auto clientId = DeviceManager::getInstance().getDeviceIdByName(client);
    sendCommand(clientId, "event", [c_eventId, param](JsonDocument &doc) {
        doc["id"] = c_eventId;
        doc["value"] = param;
    });
}


void WSCommandHandler::registerAllInternalCommands() {
    registerCommand(std::make_unique<RegisterCommand>());
    registerCommand(std::make_unique<EndActivityCommand>());

    registerCommand(std::make_unique<OnResultCommand>());

    registerCommand(std::make_unique<RemoteLogCommand>());

    registerCommand(std::make_unique<SwitchScreenCommand>());
}
