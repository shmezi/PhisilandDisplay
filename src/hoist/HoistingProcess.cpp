//
// Created by Ezra Golombek on 05/06/2026.
//

#include "HoistingProcess.h"
#include "lvgl.h"
#include "lv_api_map_v8.h"
#include "devices/DeviceManager.h"
#include "dovetail/DovetailSystem.h"
#include "logging/Logger.h"
#include "store/Store.h"
#include "ui_output/ui_FileSelection.h"
#include "ui_output/ui_HoistSystem.h"
#include "ui_output/ui_DeploySuccess.h"

#include "widgets/roller/lv_roller.h"
#include "widgets/slider/lv_slider_private.h"
#include "widgets/slider/lv_slider.h"

void HoistingProcess::onDeviceRegistration(const ClientId &id) {
    auto current = getCurrentDevice();

    DeviceManager::getInstance().addDevice(id, current.ClientId, current.file);
    xSemaphoreGive(Store::needsSave);
    lv_slider_set_value(ui_PairingProgress, 2,LV_ANIM_ON);
    if (isLastDevice()) {
        onEndOfDeployment();
        return;
    }
    currentDeviceIndex++;
    startNext();
}

void HoistingProcess::onDeviceConnect() {
    lv_slider_set_value(ui_PairingProgress, 1,LV_ANIM_ON);
}

bool HoistingProcess::isLastDevice() const {
    return currentDeviceIndex + 1 >= hoist->devices.size();
}


void HoistingProcess::onEndOfDeployment() {
    Logger::log("Successfully deployed system!");
    lv_disp_load_scr(ui_DeploySuccess);
    vTaskDelay(pdMS_TO_TICKS(3000));
    DovetailSystem::resetAllDevices();
    xSemaphoreGive(HoistSystem::getInstance().cleanupSemaphore);
}


String HoistingProcess::assignedFileForNewDevice() {
    return getCurrentDevice().file;
}


ClientConfig HoistingProcess::getCurrentDevice() {
    return hoist->devices[currentDeviceIndex];
}


void HoistingProcess::matchRollerToDevices() const {
    String options = "";
    for (auto &device: hoist->devices) {
        options += "\n"; //We anyway want the first element to be blank.
        options += device.ClientId;
    }
    lv_roller_set_options(ui_DevicesToPair, options.c_str(), LV_ROLLER_MODE_NORMAL);
}

void HoistingProcess::startNext() {
    matchScreenToCurrentDevice();
    updatePairedCountLabel();
}

void HoistingProcess::updatePairedCountLabel() const {
    auto pairedDeviceCount = hoist->id + " - " + String(currentDeviceIndex + 1) + " / " + String(hoist->devices.size());
    lv_label_set_text(ui_PairedDeviceCount, pairedDeviceCount.c_str());
}

void HoistingProcess::matchScreenToCurrentDevice() {
    const auto currentDevice = getCurrentDevice();
    lv_slider_set_value(ui_PairingProgress, 0,LV_ANIM_ON);

    lv_roller_set_selected(ui_DevicesToPair, currentDeviceIndex + 1,LV_ANIM_ON);


    lv_label_set_text(ui_Label11, currentDevice.description.c_str());
}


HoistingProcess::HoistingProcess(const std::shared_ptr<Hoist> &hoist) {
    this->hoist = hoist;
    matchRollerToDevices();
    matchScreenToCurrentDevice();
    updatePairedCountLabel();
}
