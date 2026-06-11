//
// Created by Ezra Golombek on 09/06/2026.
//

#ifndef PHISILANDDISPLAY_WSCOMMANDHANDLER_H
#define PHISILANDDISPLAY_WSCOMMANDHANDLER_H
#include <Arduino.h>
#include <ArduinoJson.h>
#include <map>
#include <memory>

#include "commands/Command.h"

class WSCommandHandler {
    static std::map<String, std::unique_ptr<Command> > commands;

public:
    static void onIncomingMessage(const uint8_t &wsClientId, JsonDocument &doc);

    static void registerCommand(std::unique_ptr<Command> command);

    static void registerAllInternalCommands();
};


#endif //PHISILANDDISPLAY_WSCOMMANDHANDLER_H
