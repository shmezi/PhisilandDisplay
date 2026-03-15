//
// Created by Ezra Golombek on 11/03/2026.
//

#include "Game.h"
#include "lvgl.h"
#include "core/lv_obj.h"
#include "misc/lv_types.h"
#include "ui_output/ui_SwingStart.h"
#include "ui_output/ui_WaterParkStart.h"
#include "ui_output/ui_FerisWheel.h"

#include <Arduino.h>

#include "lv_api_map_v8.h"
#include "dovetail/DovetailSystem.h"

#include "ui_output/ui_BlackMamba.h"
#include "widgets/arc/lv_arc.h"
#include "widgets/label/lv_label.h"

String Game::screen = "water";

void Game::onResetButton(lv_event_t *e) {
    DovetailSystem::sendMessage("reset");
}

//START / STOP BUTTON!
void Game::onStartButton(lv_event_t *e) {
    if (!(lv_obj_has_state(ui_StartStop4, LV_STATE_CHECKED) || lv_obj_has_state(ui_StartStop1, LV_STATE_CHECKED) ||
          lv_obj_has_state(ui_StartStop2, LV_STATE_CHECKED))) {
        Serial.print("~-1");

        DovetailSystem::sendMessage("event?val=-1");
        return;
    }
    if (screen == "water")
        DovetailSystem::sendMessage("event?val=-2");
    if (screen == "swings")
        DovetailSystem::sendMessage("event?val=" + String(lv_arc_get_value(ui_SpeedControl1)));
    if (screen == "blackmamba")
        DovetailSystem::sendMessage("event?val=" + String(lv_arc_get_value(ui_SpeedControl2)));
    if (screen == "ferriswheel")
        DovetailSystem::sendMessage("event?val=" + String(lv_arc_get_value(ui_SpeedControl3)));
}

void Game::setCurrentScreen() {
    if (screen == "water") {
        lv_disp_load_scr(ui_WaterParkStart);
    }
    if (screen == "swings") {
        lv_disp_load_scr(ui_SwingStart);
    }
    if (screen == "blackmamba") {
        lv_disp_load_scr(ui_BlackMamba);
    }
    if (screen == "ferriswheel") {
        lv_disp_load_scr(ui_FerisWheel);
    }
}

void Game::onBackButton(lv_event_t *e) {
    setCurrentScreen();
}

void Game::endRound() {
    lv_obj_clear_state(ui_StartStop1, LV_STATE_CHECKED);
    lv_obj_clear_state(ui_StartStop2, LV_STATE_CHECKED);
    lv_obj_clear_state(ui_StartStop4, LV_STATE_CHECKED); //Ferris wheel

    // lv_label_set_text(ui_StartStopLabel, "Start");
    lv_label_set_text(ui_StartStopLabel1, "Start");
    lv_label_set_text(ui_StartStopLabel2, "Start");
    lv_label_set_text(ui_StartStopLabel4, "Start");
    lv_obj_clear_state(ui_SpeedControl1, LV_STATE_DISABLED);
    lv_obj_clear_state(ui_SpeedControl2, LV_STATE_DISABLED);
    lv_obj_clear_state(ui_SpeedControl3, LV_STATE_DISABLED);
}

void Game::setA(const String &value) {
    lv_label_set_text(ui_SensorCValue1, value.c_str());
    lv_label_set_text(ui_SensorAValue3, value.c_str());
    lv_label_set_text(ui_BlackMambaSensorAValue, value.c_str());
    lv_label_set_text(ui_SensorAValue1, value.c_str());
}


void Game::setB(const String &value) {
    lv_label_set_text(ui_SensorCValue, value.c_str());
    lv_label_set_text(ui_SensorBValue2, value.c_str());
    lv_label_set_text(ui_BlackMambaSensorBValue, value.c_str());
    lv_label_set_text(ui_SensorBValue1, value.c_str());
}

void Game::setC(const String &value) {
    lv_label_set_text(ui_DataResult, value.c_str());
    lv_label_set_text(ui_BlackMambaSensorBValue, value.c_str());
}
