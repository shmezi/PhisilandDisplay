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



    static void loadRegistryFromSD();

    static void kickUser(uint8_t aid);


    static void wifiEvent(WiFiEvent_t event, arduino_event_info_t info);

public:
    static String ssid;
    static DNSServer dnsServer;
    static AsyncWebServer server;

    static bool needsSave;

    static String getCodeBaseForId(const String &id);

    static void saveRegistryToSD();

    static void defineRoutes();

    static void init();

    static void resetAllDevices();

    static bool connectMode;

    static void sendMessageToClient(const String &clientId, const String &message);

    static void sendMessage(const String &device, const String &message);

    static void initWifiName();

    static void resetWifi();


    static void updateDeviceCount();

    static void connection();
};


#endif //PHISILANDDISPLAY_DOVETAILSYSTEM_H
