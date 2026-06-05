//
// Created by Ezra Golombek on 17/03/2026.
//

#include "HoistSystem.h"
#include <SD.h>

#include "ArduinoJson/Deserialization/DeserializationError.hpp"
#include <ArduinoJson.h> //
#include "lvgl.h"
#include "lv_api_map_v8.h"
#include "dovetail/DovetailSystem.h"
#include "dovetail/WifiModule.h"
#include "logging/Logger.h"
#include "store/Store.h"
#include "ui_output/ui_FileSelection.h"
#include "ui_output/ui_HoistSystem.h"
#include "ui_output/ui_DeploySuccess.h"

#include "widgets/roller/lv_roller.h"
#include "widgets/slider/lv_slider_private.h"
std::map<String, Hoist> HoistSystem::hoists{};


void HoistSystem::initHoists() {
    Store::loadHoists();
    lv_roller_set_options(
        ui_FileSelector,
        Store::hoistEntriesForHoistSelection.c_str(),
        LV_ROLLER_MODE_INFINITE);
}

void HoistSystem::startDeployment() {
}


void HoistSystem::hoistLoop() {

}


void HoistSystem::registeredClient() {

}


void HoistSystem::startDeploymentWithSelected() {
    WifiModule::resetWifi();

    char selected[32];
    lv_roller_get_selected_str(ui_FileSelector, selected, sizeof(selected));

    Store::resetRegistry();


    currentHoistInDeployment = hoists[selected];

    startDeployment();
    lv_disp_load_scr(ui_HoistSystem);
    inSetup = true;
}
