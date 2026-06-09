//
// Created by Ezra Golombek on 11/03/2026.
//
#ifndef PHISILANDDISPLAY_DOVETAILSYSTEM_H
#define PHISILANDDISPLAY_DOVETAILSYSTEM_H
#include <Arduino.h>
#include <DNSServer.h>
#include <map>

#include "ESPAsyncWebServer.h"
#include "libs/asyncdns/ESPAsyncDNSServer.h"

struct FileWritePacket {
    char path[64];
    uint8_t *data;
    size_t len;
    bool isFirst;
};


class DovetailSystem {



    static std::vector<String> registeredMacsToVerify;

public:
    static AsyncDNSServer dnsServer;
    static AsyncWebServer server;

    static String getCodeBaseForId(const String &id);

    static AsyncWebSocket ws;


    static void notFound404(AsyncWebServerRequest *request);

    static void code(AsyncWebServerRequest *request);

    static void defineRoutes();

    static void macVerificationLoop();

    static void init();

    static void resetAllDevices();

    static bool connectMode;


    static void connection();
};


#endif //PHISILANDDISPLAY_DOVETAILSYSTEM_H
