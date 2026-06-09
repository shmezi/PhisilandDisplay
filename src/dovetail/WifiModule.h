//
// Created by Ezra Golombek on 02/06/2026.
//

#ifndef PHISILANDDISPLAY_WIFIMODULE_H
#define PHISILANDDISPLAY_WIFIMODULE_H
#include <WiFiGeneric.h>
#include <WString.h>


class WifiModule {
    static void initWifiName();

public:
    static void resetWifi();

    static void startWifi();

    static std::string macToString(const std::array<uint8_t, 6> &mac);

    static std::array<uint8_t, 6> parsePrettyMac(const String &macStr);

    static void updateDeviceCount();

    static void kickUserByMac(std::array<uint8_t, 6> mac);

    static String ssid;
};


#endif //PHISILANDDISPLAY_WIFIMODULE_H
