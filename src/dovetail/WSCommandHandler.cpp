//
// Created by Ezra Golombek on 09/06/2026.
//

#include "WSCommandHandler.h"

#include <map>
#include <memory>

#include "commands/Command.h"
#include "commands/RegisterCommand.h"
#include "logging/Logger.h"
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

void WSCommandHandler::registerAllInternalCommands() {
    registerCommand(std::make_unique<RegisterCommand>());
}
