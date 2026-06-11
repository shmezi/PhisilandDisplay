//
// Created by Ezra Golombek on 12/06/2026.
//

#include "SwitchScreenCommand.h"

#include "game/Game.h"

String SwitchScreenCommand::name() {
    return "screen";
}

void SwitchScreenCommand::execute(const uint8_t &wsClientId, JsonDocument doc) {
    String screenName = doc["name"];
    Game::screen = screenName;
    Game::setCurrentScreen();
}
