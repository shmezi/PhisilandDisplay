#include <lvgl.h>
#include <TFT_eSPI.h>
#include <WebServer.h>
#include <Arduino.h>
#include <esp_task_wdt.h>

#include "display/Display.h"
#include "dovetail/DovetailSystem.h"
#include "game/Game.h"
#include "hoist/HoistSystem.h"
#include "store/Store.h"
#include "ui_output/ui_FileSelection.h"
#include "ui_output/ui_SplashScreen.h"
#include "ui_output/ui_SudoMode.h"

extern "C" {
void onStartButton(lv_event_t *e) {
    Game::onStartButton(e);
}

void onResetButton(lv_event_t *e) {
    Game::onResetButton(e);
}

void onDeploy(lv_event_t *e) {
    HoistSystem::startDeploymentWithSelected();
}

void validateSudoCode(lv_event_t *e) {
    const auto text = String(lv_textarea_get_text(ui_Code));
    if (text == "314159") {
        lv_disp_load_scr(ui_FileSelection);
    }
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
    esp_task_wdt_init(5, true);
    Store::initValuesFromSD();

    DovetailSystem::init();
    Display::innit();

    const auto codeBaseNameWithExtension = DovetailSystem::getCodeBaseForId("core");
    const auto codeBaseName = codeBaseNameWithExtension.substring(0, codeBaseNameWithExtension.length() - 5);

    lv_label_set_text(ui_DeviceId, ("Connecting to: " + codeBaseName).c_str());


    const auto ssid = "SSID: " + DovetailSystem::ssid;
    lv_label_set_text(ui_SSID, ssid.c_str());
    HoistSystem::initHoists();

    Serial.println("Setup done");
}


char receivedChars[64]; // Buffer to store the received data

void loop() {
    Display::lvglTask();
    DovetailSystem::dnsServer.processNextRequest();
    DovetailSystem::saveRegistryToSD();
    Game::setCurrentScreen();
    Game::updateValues();
    HoistSystem::hoistLoop();
    if (Game::shouldEndActivity) {
        Game::endRound();
        Game::shouldEndActivity = false;
    }
    // DovetailSystem::server.handleClient();
    delay(5);
}
