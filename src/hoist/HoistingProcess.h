//
// Created by Ezra Golombek on 05/06/2026.
//

#ifndef PHISILANDDISPLAY_HOISTINGPROCESS_H
#define PHISILANDDISPLAY_HOISTINGPROCESS_H
#include <memory>

#include "HoistStructures.h"
#include "devices/ClientId.h"


struct Hoist;

class HoistingProcess {
    std::shared_ptr<Hoist> hoist;
    int currentDeviceIndex = 0;


    bool isLastDevice() const;

    void onEndOfDeployment();


    ClientConfig getCurrentDevice();

    void matchRollerToDevices() const;

    void startNext();

    void updatePairedCountLabel() const;

    void matchScreenToCurrentDevice();

public:
    void onDeviceRegistration(const ClientId &id);

    void onDeviceConnect();


    String assignedFileForNewDevice();

    explicit HoistingProcess(const std::shared_ptr<Hoist> &hoist);
};


#endif //PHISILANDDISPLAY_HOISTINGPROCESS_H
