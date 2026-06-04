//
// Created by Ezra Golombek on 11/03/2026.
//
#include <Arduino.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <map>

#include "ESPAsyncWebServer.h"
#ifndef PHISILANDDISPLAY_DOVETAILSYSTEM_H
#define PHISILANDDISPLAY_DOVETAILSYSTEM_H

struct FileWritePacket {
    char path[64];
    uint8_t *data;
    size_t len;
    bool isFirst;
};


class DovetailSystem {
    static void kickUserWithMac(const String &macToEvict);


    static std::vector<String> registeredMacsToVerify;

public:
    static String ssid;
    static DNSServer dnsServer;
    static AsyncWebServer server;

    static bool needsSave;

    static std::map<String, IPAddress> macToIp;

    static std::map<String, String> macToName;

    static std::map<String, String> nameToMac;

    static std::map<String, String> macToCode;

    static String getCodeBaseForId(const String &id);


    static void defineRoutes();

    static void connectionLoop();

    static void init();

    static void resetAllDevices();

    static bool connectMode;

    static void sendMessageToClient(const String &clientId, const String &message);

    static void sendMessage(const String &device, const String &message);


    static void connection();
};


#endif //PHISILANDDISPLAY_DOVETAILSYSTEM_H
