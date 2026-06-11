//
// Created by Ezra Golombek on 12/06/2026.
//

#include "OnResultCommand.h"

#include "game/Game.h"

String OnResultCommand::name() {
    return "result";
}

void OnResultCommand::execute(const uint8_t &wsClientId, JsonDocument doc) {
    if (doc["id"] == "a")
        Game::setA(doc["value"]);
    if (doc["id"] == "b")
        Game::setB(doc["value"]);
    if (doc["id"] == "b")
        Game::setC(doc["value"]);
}
