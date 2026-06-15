//
// Created by Ezra Golombek on 17/03/2026.
//

#ifndef PHISILANDDISPLAY_HOISTSYSTEM_H
#define PHISILANDDISPLAY_HOISTSYSTEM_H

#include <map>
#include <memory>
#include <vector>
#include <WString.h>

#include "HoistingProcess.h"
#include "dovetail/WSCommandHandler.h"


class HoistSystem {
    std::map<String, std::shared_ptr<Hoist> > hoists;

    std::shared_ptr<HoistingProcess> process;

    void startDeployment(const String &hoist);


    HoistSystem() {
        cleanupSemaphore = xSemaphoreCreateBinary();
        hoists = {};
        process = nullptr;
    }

    ~HoistSystem() {
    }

public:
    SemaphoreHandle_t cleanupSemaphore;

    bool hasFinishedDeployment();

    HoistSystem(const HoistSystem &) = delete;

    HoistSystem &operator=(const HoistSystem &) = delete;

    void onDeviceConnect();

    void killProcess();

    static HoistSystem &getInstance() {
        static HoistSystem instance;
        return instance;
    }

    void initHoistSystem();

    bool isHoisting() const;

    String assignedFileForNewDevice() const;

    bool onDeviceRegistration(const ClientId &id) const;


    void loadHoist(const Hoist &hoist);

    std::shared_ptr<Hoist> getHoistById(const String &id);

    void startDeploymentWithSelected();
};


#endif //PHISILANDDISPLAY_HOISTSYSTEM_H
