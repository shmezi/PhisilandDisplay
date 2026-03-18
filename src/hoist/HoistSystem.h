//
// Created by Ezra Golombek on 17/03/2026.
//

#ifndef PHISILANDDISPLAY_HOISTSYSTEM_H
#define PHISILANDDISPLAY_HOISTSYSTEM_H

#include <map>
#include <vector>
#include <WString.h>


struct Script {
    String deviceId;
    String description;
    String file;
};

struct Deployment {
    String id;
    String description;
    std::vector<Script> devices;
};

class HoistSystem {
    static std::map<String, Deployment> hoists;


public:
    static Deployment deployment;
    static int deviceIndex;
    static void initHoists();

    static void loadCurrentDevice();

    static void connectedNewClient();

    static int hoistStatus;

    static void hoistLoop();

    static void registeredClient();

    static void startDeploymentWithSelected();
    static bool inSetup;
};


#endif //PHISILANDDISPLAY_HOISTSYSTEM_H
