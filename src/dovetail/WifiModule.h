//
// Created by Ezra Golombek on 02/06/2026.
//

#ifndef PHISILANDDISPLAY_WIFIMODULE_H
#define PHISILANDDISPLAY_WIFIMODULE_H
#include <WiFiGeneric.h>
#include <WString.h>


class WifiModule {
    static String ssid;

    static void initWifiName();

    static void updateDeviceCount();

    static void wifiEvent(arduino_event_id_t event, arduino_event_info_t info);

    static void resetWifi();
};


#endif //PHISILANDDISPLAY_WIFIMODULE_H