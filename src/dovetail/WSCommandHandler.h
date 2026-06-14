//
// Created by Ezra Golombek on 09/06/2026.
//

#ifndef PHISILANDDISPLAY_WSCOMMANDHANDLER_H
#define PHISILANDDISPLAY_WSCOMMANDHANDLER_H
#include <Arduino.h>
#include <ArduinoJson.h>
#include <map>
#include <memory>

#include "DovetailSystem.h"
#include "commands/Command.h"
#include "store/Store.h"

class AsyncWebSocketClient;

class WSCommandHandler {
    static std::map<String, std::unique_ptr<Command> > commands;

public:
    static void onIncomingMessage(const uint8_t &wsClientId, JsonDocument &doc);

    static void registerCommand(std::unique_ptr<Command> command);

    template<class F>
    static void sendCommand(const ClientId &mac, std::string command, F changes);

    static void startEvent(const String &client, const String &eventId, int param);

    static void registerAllInternalCommands();
};

template<typename F>
void WSCommandHandler::sendCommand(const ClientId &mac, std::string command, F changes) {
    JsonDocument doc;

    doc["command"] = command;
    changes(doc);
    String serialized;
    serializeJson(doc, serialized);
    if (AsyncWebSocketClient *client = DovetailSystem::getWSClientByMac(mac))
        client->text(serialized);
}

#endif //PHISILANDDISPLAY_WSCOMMANDHANDLER_H
