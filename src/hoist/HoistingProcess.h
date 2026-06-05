//
// Created by Ezra Golombek on 05/06/2026.
//

#ifndef PHISILANDDISPLAY_HOISTINGPROCESS_H
#define PHISILANDDISPLAY_HOISTINGPROCESS_H
#include "HoistSystem.h"


class HoistingProcess {
    Hoist hoist;
    int currentDeviceIndex = 0;

    HoistingProcess(const Hoist &hoist) {
        this->hoist = hoist;
    }

    void onNewClientConnected();

    bool isLastDevice() const;

    void onEndOfDeployment();

    void onClientRegistration();

    ClientConfig getCurrentDevice();

    void matchRollerToDevices() const;

    void startNext();

    void matchScreenToCurrentDevice();
};


#endif //PHISILANDDISPLAY_HOISTINGPROCESS_H
