//
// Created by Ezra Golombek on 12/06/2026.
//

#include "RemoteLogCommand.h"

#include "logging/Logger.h"

String RemoteLogCommand::name() {
    return "log";
}

void RemoteLogCommand::execute(const uint8_t &wsClientId, JsonDocument doc) {
    Logger::remoteLog(doc["message"]);
}
