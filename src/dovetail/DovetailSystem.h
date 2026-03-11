//
// Created by Ezra Golombek on 11/03/2026.
//
#include <Arduino.h>
#include <WebServer.h>
#ifndef PHISILANDDISPLAY_DOVETAILSYSTEM_H
#define PHISILANDDISPLAY_DOVETAILSYSTEM_H


class DovetailSystem {
    static void kickUser(uint8_t aid);

    static void wifiEvent(WiFiEvent_t event, arduino_event_info_t info);

    static void handleCode();

    static void handlePin();



public:
    static WebServer server;
    static void init();
    static bool connectMode;

    static void connection();
};


#endif //PHISILANDDISPLAY_DOVETAILSYSTEM_H
