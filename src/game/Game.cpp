//
// Created by Ezra Golombek on 11/03/2026.
//

#include "Game.h"
#include "lvgl.h"
#include "core/lv_obj.h"
#include "misc/lv_types.h"
#include "ui_output/ui_BlackMambaStart.h"
#include "ui_output/ui_SwingStart.h"
#include "ui_output/ui_WaterParkStart.h"
#include <Arduino.h>

#include "lv_api_map_v8.h"
#include "dovetail/DovetailSystem.h"
#include "widgets/arc/lv_arc.h"
#include "widgets/label/lv_label.h"

int Game::screen = 1;

void Game::onResetButton(lv_event_t *e) {
    DovetailSystem::sendMessage();
}
void Game::onStartButton(lv_event_t *e) {

    if (!(lv_obj_has_state(ui_StartStop, LV_STATE_CHECKED) || lv_obj_has_state(ui_StartStop1, LV_STATE_CHECKED) ||
          lv_obj_has_state(ui_StartStop2, LV_STATE_CHECKED))) {
        Serial.print("~-1");
        return;
    }

    switch (screen) {
        case 1:
            Serial.print("~-2");
            break;
        case 2:
            Serial.print(String("~") + String(lv_arc_get_value(ui_SpeedControl1)));
            break;
        case 3:
            Serial.print(String("~") + String(lv_arc_get_value(ui_SpeedControl2)));
            break;
    }
}

void Game::setCurrentScreen() {
    if (screen == 1) {
        lv_disp_load_scr(ui_WaterParkStart);
    }
    if (screen == 2) {
        lv_disp_load_scr(ui_SwingStart);
    }
    if (screen == 3) {
        lv_disp_load_scr(ui_BlackMambaStart);
    }
}

void Game::onBackButton(lv_event_t *e) {
    setCurrentScreen();
}

void Game::endRound() {
    lv_obj_clear_state(ui_StartStop, LV_STATE_CHECKED);

    lv_obj_clear_state(ui_StartStop1, LV_STATE_CHECKED);
    lv_obj_clear_state(ui_StartStop2, LV_STATE_CHECKED);
    lv_label_set_text(ui_StartStopLabel, "Start");
    lv_label_set_text(ui_StartStopLabel1, "Start");
    lv_label_set_text(ui_StartStopLabel2, "Start");

    lv_obj_clear_state(ui_SpeedControl1, LV_STATE_DISABLED);
    lv_obj_clear_state(ui_SpeedControl2, LV_STATE_DISABLED);
}

void Game::setA(int value) {
    auto finalValue = String(value);
    lv_label_set_text(ui_SensorAValue, (finalValue + "ms").c_str());
    lv_label_set_text(ui_SensorAValue1, (finalValue + "°").c_str());
    lv_label_set_text(ui_SensorAValue2, (finalValue + "ms").c_str());
}


void Game::setB(int value) {
    auto finalValue = String(value);
    lv_label_set_text(ui_SensorBValue, (finalValue + "ms").c_str());
    lv_label_set_text(ui_SensorBValue1, (finalValue + "°").c_str());
}

void Game::setC(int value) {
    auto finalValue = String(value);
    lv_label_set_text(ui_SensorCValue, (finalValue + "ms").c_str());

    lv_label_set_text(ui_DataResult, (finalValue + " Rotations").c_str());
}
