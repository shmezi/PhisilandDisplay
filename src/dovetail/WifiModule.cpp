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
#include <esp_wifi.h>

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



void WifiModule::kickUserByMac(ClientId mac) {
    wifi_sta_list_t connectedClients;
    esp_wifi_ap_get_sta_list(&connectedClients);

    for (int i = 0; i < connectedClients.num; i++) {
        const wifi_sta_info_t client = connectedClients.sta[i];
        uint16_t aidOfClientToKick = 0;
        if (memcmp(mac.data(), client.mac, 6) == 0) {
            esp_wifi_ap_get_sta_aid(client.mac, &aidOfClientToKick);
            esp_wifi_deauth_sta(aidOfClientToKick);
            break;
        }
    }
}



void WifiModule::startWifi() {
    initWifiName();
    WiFi.softAP(ssid, password, 1, 0, 5);


    wifi_config_t conf;
    esp_wifi_get_config(WIFI_IF_AP, &conf);
    conf.ap.pairwise_cipher = WIFI_CIPHER_TYPE_CCMP;
    esp_wifi_set_config(WIFI_IF_AP, &conf);




    IPAddress apIP = WiFi.softAPIP();
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

    WiFi.setSleep(false);

    // WiFi.onEvent(wifiEvent);


    // DovetailSystem::dnsServer.setTTL(300);
    // DovetailSystem::dnsServer.start(53, "am.it", WiFi.softAPIP());
    DovetailSystem::server.addHandler(&DovetailSystem::ws);
    DovetailSystem::server.begin();
}

std::string WifiModule::macToString(const ClientId &mac) {
    char buf[18];
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return buf;
}

ClientId WifiModule::parsePrettyMac(const String &macStr) {
    ClientId mac;
    sscanf(macStr.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
    return mac;
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
