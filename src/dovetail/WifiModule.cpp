//
// Created by Ezra Golombek on 02/06/2026.
//

#include "WifiModule.h"

#include <SD.h>
#include <WString.h>
#include "DovetailSystem.h"
#include <HTTPClient.h>
#include <DNSServer.h>
#include <esp_wifi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <map>
#include <SD.h>
#include "game/Game.h"
#include "hoist/HoistSystem.h"
#include "store/Store.h"
#include "ui_output/ui_FileSelection.h"
#include "widgets/label/lv_label.h"
const char *verbs[] = {
    "Carve",
    "Shape",
    "Cut",
    "Split",
    "Sand",
    "Plant",
    "Grow",
    "Climb",
    "Track",
    "Forge",
    "Bend",
    "Lash",
    "Stack",
    "Scout",
    "Tend"
};

const char *nouns[] = {
    "Wood",
    "Oak",
    "Pine",
    "Bark",
    "Roots",
    "Trail",
    "Grove",
    "Stone",
    "Fire",
    "Axe",
    "Camp",
    "Ridge",
    "Creek",
    "Fern",
    "Moss"
};
String WifiModule::ssid = "Dovetail-ERROR-SD-NOT-INSERTED!";
auto password = "Phisiland";




void WifiModule::initWifiName() {
    if (SD.exists("/wifi-name.txt")) {
        File f = SD.open("/wifi-name.txt", FILE_READ);
        if (f) {
            ssid = f.readString();
            f.close();
        }
        return;
    }
    File f = SD.open("/wifi-name.txt", FILE_WRITE);
    const int rand1 = random(0, 14);
    const int rand2 = random(0, 14);

    const String newWifiName = "Dovetail-" + String(verbs[rand1]) + "-" + String(nouns[rand2]);
    if (f) {
        f.print(newWifiName);
        f.close();
        Serial.println("Stored wifi name: " + newWifiName);
        ssid = newWifiName;
    }
}


void WifiModule::updateDeviceCount() {
    const uint8_t numStations = WiFi.softAPgetStationNum();
    String text = "Connected clients: ";
    text += String(numStations);
    lv_label_set_text(ui_ConnectedLabel, text.c_str());
}

void WifiModule::wifiEvent(WiFiEvent_t event, arduino_event_info_t info) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP: {
            // For the ESP32's own MAC when it gets an IP
            Serial.print("ESP32 Station MAC: ");
            Serial.println(WiFi.macAddress());
            break;
        }

        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED: {
            updateDeviceCount();
        }
        default:
            break;
    }
}

void WifiModule::resetWifi() {
    if (SD.exists("/wifi-name.txt")) {
        SD.remove("/wifi-name.txt");
    }
    initWifiName();
    WiFiClass::mode(WIFI_OFF);

    WiFi.softAP(ssid, password);
    DovetailSystem::macToCode.clear();
    DovetailSystem::macToIp.clear();
    DovetailSystem::macToName.clear();
    DovetailSystem::nameToMac.clear();
    Store::allowedMacs.clear();
}
