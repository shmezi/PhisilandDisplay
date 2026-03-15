//
// Created by Ezra Golombek on 11/03/2026.
//
#include <Arduino.h>
#include <WebServer.h>
#include <DNSServer.h>

#include "ESPAsyncWebServer.h"
#ifndef PHISILANDDISPLAY_DOVETAILSYSTEM_H
#define PHISILANDDISPLAY_DOVETAILSYSTEM_H


class DovetailSystem {
    static String ssid;

    static void kickUser(uint8_t aid);


    static void wifiEvent(WiFiEvent_t event, arduino_event_info_t info);

    static String lastRunFile;

public:
    static DNSServer dnsServer;
    static AsyncWebServer server;


    static void defineRoutes();

    static void init();

    static bool connectMode;

    static void sendMessage(const String &message);

    static void initWifiName();

    static void saveLastRun(const String &filename);

    static void connection();
};


#endif //PHISILANDDISPLAY_DOVETAILSYSTEM_H
