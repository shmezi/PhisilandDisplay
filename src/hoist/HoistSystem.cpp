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
std::map<String, Deployment> HoistSystem::hoists{};
Deployment HoistSystem::deployment;
int HoistSystem::deviceIndex = 0;
bool HoistSystem::inSetup = false;

void HoistSystem::initHoists() {
    if (!SD.exists("/hoists")) {
        SD.mkdir("/hoists");
        return;
    }
    auto hoistsDirectory = SD.open("/hoists");

    String names = "";
    auto file = hoistsDirectory.openNextFile();
    while (file) {
        if (file.isDirectory() || String(file.name()).startsWith("._")) {
            file = hoistsDirectory.openNextFile();
            continue;
        }

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, file); //
        if (error) {
            String errMsg = "Error reading hoist file: '" + String(file.name()) + "'!";
            Serial.println(errMsg);
            continue;
        }
        auto name = doc["name"].as<String>();
        if (names != "") names += "\n";
        names += name;
        const auto description = doc["description"].as<String>();
        std::vector<Script> devices{};
        auto deviceList = doc["devices"].as<JsonArray>();
        for (const auto &rawDevice: deviceList) {
            const auto device = rawDevice.as<JsonObject>();
            const auto deviceId = device["id"].as<String>();
            const auto deviceDescription = device["description"].as<String>();
            const auto deviceFile = device["file"].as<String>();
            devices.push_back({deviceId, deviceDescription, deviceFile});
        }
        hoists[name] = {name, description, devices};


        file = hoistsDirectory.openNextFile();
    }
    lv_roller_set_options(ui_FileSelector,
                          names.c_str(),
                          LV_ROLLER_MODE_INFINITE);

    hoistsDirectory.close();
}

void HoistSystem::loadCurrentDevice() {
    lv_roller_set_selected(ui_DevicesToPair, deviceIndex + 1,LV_ANIM_ON);
    String pairedCont = deployment.id + " - " + String(deviceIndex + 1) + " / " + String(deployment.devices.size());
    lv_label_set_text(ui_PairedDeviceCount, pairedCont.c_str());
    lv_slider_set_value(ui_PairingProgress, 0,LV_ANIM_ON);
    lv_label_set_text(ui_Label11, deployment.devices[deviceIndex].description.c_str());
}

void HoistSystem::connectedNewClient() {
    lv_slider_set_value(ui_PairingProgress, 1,LV_ANIM_ON);
}

int HoistSystem::hoistStatus = -1;

void HoistSystem::hoistLoop() {
    if (hoistStatus == -1) return;
    if (hoistStatus == 1) connectedNewClient();
    if (hoistStatus == 2) registeredClient();
}


void HoistSystem::registeredClient() {
    hoistStatus = 0;
    lv_slider_set_value(ui_PairingProgress, 2,LV_ANIM_ON);
    if (deviceIndex + 1 >= deployment.devices.size()) {
        inSetup = false;
        hoistStatus = -1;
        DovetailSystem::resetAllDevices();
        lv_disp_load_scr(ui_DeploySuccess);
        return;
    }

    deviceIndex++;
    loadCurrentDevice();
}


void HoistSystem::startDeploymentWithSelected() {
    hoistStatus = 0;
    DovetailSystem::resetWifi();

    char selected[32];
    lv_roller_get_selected_str(ui_FileSelector, selected, sizeof(selected));
    if (SD.exists("/allowed-mac.txt")) {
        SD.remove("/allowed-mac.txt");
    }
    auto file = SD.open("/allowed-mac.txt",FILE_WRITE);
    file.close();
    deployment = hoists[selected];

    String options = "";
    for (auto &device: deployment.devices) {
        options += "\n"; //We anyways want the first element to be blank.
        options += device.deviceId;
    }
    lv_roller_set_options(ui_DevicesToPair, options.c_str(), LV_ROLLER_MODE_NORMAL);
    loadCurrentDevice();
    lv_disp_load_scr(ui_HoistSystem);
    inSetup = true;
}
