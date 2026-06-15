//
// Created by Ezra Golombek on 15/06/2026.
//

#ifndef PHISILANDDISPLAY_HOISTSTRUCTURES_H
#define PHISILANDDISPLAY_HOISTSTRUCTURES_H
#include <vector>
#include <WString.h>
/*
 * Configuration for an individual config.
 */
struct ClientConfig {
    String ClientId;
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
#endif //PHISILANDDISPLAY_HOISTSTRUCTURES_H
