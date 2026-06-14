//
// Created by Ezra Golombek on 02/06/2026.
//

#ifndef PHISILANDDISPLAY_WIFIMODULE_H
#define PHISILANDDISPLAY_WIFIMODULE_H
#include <WiFiGeneric.h>
#include <WString.h>

#include "devices/ClientId.h"


class WifiModule {
    static void initWifiName();

public:
    static void resetWifi();

    static void startWifi();

    static std::string macToString(const ClientId &mac);

    static ClientId parsePrettyMac(const String &macStr);

    static void updateDeviceCount();

    static void kickUserByMac(ClientId mac);

    static String ssid;
};


#endif //PHISILANDDISPLAY_WIFIMODULE_H
