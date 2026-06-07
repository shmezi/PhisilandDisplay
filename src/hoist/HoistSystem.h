//
// Created by Ezra Golombek on 17/03/2026.
//

#ifndef PHISILANDDISPLAY_HOISTSYSTEM_H
#define PHISILANDDISPLAY_HOISTSYSTEM_H

#include <map>
#include <vector>
#include <WString.h>

/*
 * Configuration for an individual config.
 */
struct ClientConfig {
    String deviceId;
    String description;
    String file;
};

/**
 * Configuration for a hoist that can be deployed.
 */
struct Hoist {
    String id;
    String description;
    std::vector<ClientConfig> devices;
};

enum HoistStatus {
    NONCONNECTED,
    CONNECTED,
    REGISTERED
};

class HoistSystem {
public:
    static std::map<String, Hoist> hoists;
    static int deviceIndex;

    static void initHoists();

    static void startDeployment();

    static HoistStatus status;

    static void hoistLoop();

    static void registeredClient();

    static void startDeploymentWithSelected();

};


#endif //PHISILANDDISPLAY_HOISTSYSTEM_H
