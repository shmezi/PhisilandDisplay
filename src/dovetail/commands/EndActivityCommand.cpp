//
// Created by Ezra Golombek on 12/06/2026.
//

#include "EndActivityCommand.h"

#include "game/Game.h"

String EndActivityCommand::name() {
    return "endActivity";
}

void EndActivityCommand::execute(const uint8_t &wsClientId, JsonDocument doc) {
Game::endRound();
}
