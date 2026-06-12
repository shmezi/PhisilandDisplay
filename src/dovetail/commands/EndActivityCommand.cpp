//
// Created by Ezra Golombek on 12/06/2026.
//

#include "EndActivityCommand.h"

#include "game/Game.h"
#include "logging/Logger.h"

String EndActivityCommand::name() {
    return "endActivity";
}

void EndActivityCommand::execute(const uint8_t &wsClientId, JsonDocument doc) {
    Logger::log("Ending round!");
    Game::endRound();
}
