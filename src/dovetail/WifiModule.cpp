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

String generateWifiName() {
    const auto verb = verbs[random(0, 14)];
    const auto noun = nouns[random(0, 14)];
    return String("Dovetail-") + verb + "-" + noun;
}

/*
 * Ensure that a wifi name exists for this device.
 */
void WifiModule::initWifiName() {
    Store::getFileOrCreateDefault("wifi-name.txt", [](File &f) {
        f.print(generateWifiName());
        f.close();
        return true;
    });
    ssid = Store::readFileToString("wifi-name.txt");
}


void WifiModule::updateDeviceCount() {
    const uint8_t clientCount = WiFi.softAPgetStationNum();
    auto updatedConnectedClientsLabel = String("Connected clients: ") + clientCount;

    lv_label_set_text(ui_ConnectedLabel, updatedConnectedClientsLabel.c_str());
}

void WifiModule::startWifi() {
    initWifiName();
    WiFi.softAP(ssid, password, 1, 0, 1);
    IPAddress apIP = WiFi.softAPIP();
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

    WiFi.setSleep(false);
    // WiFi.onEvent(wifiEvent);

    DovetailSystem::dnsServer.setTTL(300);
    DovetailSystem::dnsServer.start(53, "am.it", WiFi.softAPIP());


    DovetailSystem::server.begin();
}

void WifiModule::resetWifi() {
    Store::ensureDeleted("wifi-name.txt");
    initWifiName();

    WiFiClass::mode(WIFI_OFF);

    WiFi.softAP(ssid, password);

    WiFi.setSleep(false);
    // WiFi.onEvent(wifiEvent);

    DovetailSystem::dnsServer.stop();
    DovetailSystem::server.end();
    startWifi();
}
