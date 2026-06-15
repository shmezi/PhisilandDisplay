//
// Created by Ezra Golombek on 17/03/2026.
//

#include "HoistSystem.h"
#include <SD.h>

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

void HoistSystem::onDeviceConnect() {
    if (process)
        process->onDeviceConnect();
}

void HoistSystem::killProcess() {
    process = nullptr;
}


void HoistSystem::initHoistSystem() {
    Store::loadHoists();
    lv_roller_set_options(
        ui_FileSelector,
        Store::hoistEntriesForHoistSelection.c_str(),
        LV_ROLLER_MODE_INFINITE);
}

bool HoistSystem::isHoisting() const {
    return process != nullptr;
}

String HoistSystem::assignedFileForNewDevice() const {
    if (process)
        return process->assignedFileForNewDevice();
    return "error.ezra";
}


bool HoistSystem::onDeviceRegistration(const ClientId &id) const {
    if (process != nullptr) {
        process->onDeviceRegistration(id);
        return true;
    }
    return false;
}


void HoistSystem::loadHoist(const Hoist &hoist) {
    hoists[hoist.id] = std::make_shared<Hoist>(hoist);
}

std::shared_ptr<Hoist> HoistSystem::getHoistById(const String &id) {
    return hoists[id];
}

void HoistSystem::startDeployment(const String &hoist) {
    process = std::make_shared<HoistingProcess>(getHoistById(hoist));
}

bool HoistSystem::hasFinishedDeployment() {
    return xSemaphoreTake(HoistSystem::getInstance().cleanupSemaphore, portMAX_DELAY) == pdTRUE;
}


void hoistVerifyEnd(void *pvParameters) {
    while (true) {
        // Task sleeps here until the semaphore is given
        if (HoistSystem::getInstance().hasFinishedDeployment()) {
            vTaskDelay(pdMS_TO_TICKS(5000)); //Delay to make sure devices are reset!
            if (HoistSystem::getInstance().isHoisting()) {
                HoistSystem::getInstance().killProcess();
            }
            vTaskDelete(nullptr);
        }
    }
}

void HoistSystem::startDeploymentWithSelected() {
    WifiModule::resetWifi();
    Store::resetRegistry();
    char selected[32];
    lv_roller_get_selected_str(ui_FileSelector, selected, sizeof(selected));
    xTaskCreatePinnedToCore(
        hoistVerifyEnd,
        "DeployTask",
        4096,
        nullptr,
        3,
        NULL,
        1
    );


    startDeployment(selected);
    lv_disp_load_scr(ui_HoistSystem);
    // inSetup = true;
}
