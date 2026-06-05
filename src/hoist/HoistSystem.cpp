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
#include "ui_output/ui_FileSelection.h"
#include "ui_output/ui_HoistSystem.h"
#include "ui_output/ui_DeploySuccess.h"

#include "widgets/roller/lv_roller.h"
#include "widgets/slider/lv_slider_private.h"
std::map<String, Hoist> HoistSystem::hoists{};
HoistStatus HoistSystem::status = -1;

void HoistSystem::initHoists() {


    lv_roller_set_options(ui_FileSelector,
                          names.c_str(),
                          LV_ROLLER_MODE_INFINITE);


}

void HoistSystem::loadCurrentDevice() {
    lv_roller_set_selected(ui_DevicesToPair, deviceIndex + 1,LV_ANIM_ON);
    String pairedCont = currentHoistInDeployment.id + " - " + String(deviceIndex + 1) + " / " + String(currentHoistInDeployment.devices.size());
    lv_label_set_text(ui_PairedDeviceCount, pairedCont.c_str());
    lv_slider_set_value(ui_PairingProgress, 0,LV_ANIM_ON);
    lv_label_set_text(ui_Label11, currentHoistInDeployment.devices[deviceIndex].description.c_str());
}

void HoistSystem::connectedNewClient() {
    lv_slider_set_value(ui_PairingProgress, 1,LV_ANIM_ON);
}



void HoistSystem::hoistLoop() {
    if (status == -1) return;
    if (status == 1) connectedNewClient();
    if (status == 2) registeredClient();
}


void HoistSystem::registeredClient() {
    status = 0;
    lv_slider_set_value(ui_PairingProgress, 2,LV_ANIM_ON);
    if (deviceIndex + 1 >= currentHoistInDeployment.devices.size()) {
        Serial.println("Successfully deployed system!");
        lv_disp_load_scr(ui_DeploySuccess);
        inSetup = false;
        status = -1;
        DovetailSystem::resetAllDevices();

        return;
    }

    deviceIndex++;
    loadCurrentDevice();
}


void HoistSystem::startDeploymentWithSelected() {
    status = 0;
    DovetailSystem::resetWifi();

    char selected[32];
    lv_roller_get_selected_str(ui_FileSelector, selected, sizeof(selected));
    if (SD.exists("/allowed-mac.txt")) {
        SD.remove("/allowed-mac.txt");
    }
    auto file = SD.open("/allowed-mac.txt",FILE_WRITE);
    file.close();
    currentHoistInDeployment = hoists[selected];

    String options = "";
    for (auto &device: currentHoistInDeployment.devices) {
        options += "\n"; //We anyways want the first element to be blank.
        options += device.deviceId;
    }
    lv_roller_set_options(ui_DevicesToPair, options.c_str(), LV_ROLLER_MODE_NORMAL);
    loadCurrentDevice();
    lv_disp_load_scr(ui_HoistSystem);
    inSetup = true;
}
