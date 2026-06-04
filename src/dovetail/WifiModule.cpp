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


void WifiModule::loadRegistryFromSD() {
    if (!SD.exists("/config.json")) {
        Serial.println("ℹ️ config.json not found. Initializing empty file.");
        saveRegistryToSD();
        return;
    }

    File file = SD.open("/config.json", FILE_READ);
    if (file.size() == 0) {
        file.close();
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.printf("❌ JSON Parse Error: %s\n", error.c_str());
        return;
    }

    // Clear current RAM maps to prevent data duplication/stale entries
    DovetailSystem::macToName.clear();
    DovetailSystem::macToCode.clear();

    JsonObject root = doc.as<JsonObject>();
    for (JsonPair p: root) {
        String mac = p.key().c_str(); // MAC is the primary key in JSON
        JsonObject data = p.value();

        // Restore macToCode Map
        if (data.containsKey("code")) {
            DovetailSystem::macToCode[mac] = data["code"].as<String>();
        }

        // Restore nameToMac Map (Name is Key, MAC is Value)
        if (data.containsKey("name")) {
            String name = data["name"].as<String>();
            DovetailSystem::macToName[mac] = name;
            DovetailSystem::nameToMac[name] = mac;
        }
    }
    Serial.printf("✅ Successfully loaded %d devices from SD.\n", (int) DovetailSystem::macToCode.size());
}

void WifiModule::saveRegistryToSD() {
    if (!DovetailSystem::needsSave) return;
    JsonDocument doc;

    // We use macToCode as the primary loop because every registered
    // device should have a code assignment (even if it's "default.ezra")
    for (auto const &[mac, code]: DovetailSystem::macToCode) {
        doc[mac]["code"] = code;

        // Check if this MAC also has a nickname in nameToMac
        // We iterate through nameToMac to find the matching MAC
        for (auto const &[m, name]: DovetailSystem::macToName) {
            if (m == mac) {
                doc[mac]["name"] = name;
                break;
            }
        }
    }

    File file = SD.open("/config.json", FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open config for writing");
        return;
    }

    serializeJson(doc, file);
    file.close();
    DovetailSystem::needsSave = false;
    Serial.println("Phisiland config saved to SD.");
}


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
