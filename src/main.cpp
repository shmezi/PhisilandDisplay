#include <lvgl.h>
#include <TFT_eSPI.h>
#include <WebServer.h>
#include <Arduino.h>
#include "display/Display.h"
#include "dovetail/DovetailSystem.h"
#include "game/Game.h"
#include "store/Store.h"

extern "C" {
void onStartButton(lv_event_t *e) {
    Game::onStartButton(e);
}
    void onResetButton(lv_event_t *e) {
    Game::onResetButton(e);
}

void onBackButton(lv_event_t *e) {
    Game::onBackButton(e);
}
    void connectBT(lv_event_t *e) {

    DovetailSystem::connection();
}
}


void setup() {
    Serial.begin(115200);

    Store::initValuesFromSD();

    DovetailSystem::init();
    Display::innit();

    Serial.println("Setup done");
}


char receivedChars[64]; // Buffer to store the received data

void loop() {
    Display::lvglTask();
    // DovetailSystem::server.handleClient();
    delay(5);
}
