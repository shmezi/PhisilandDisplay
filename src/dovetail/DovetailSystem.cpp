//
// Created by Ezra Golombek on 11/03/2026.
//

#include "DovetailSystem.h"

#include <esp_wifi.h>
#include <WebServer.h>

#include "store/Store.h"
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

int rand1 = random(0, 30);
int rand2 = random(0, 30);

String ssid = "Dovetail-" + String(verbs[rand1]) + "-" + String(nouns[rand2]);
const char *password = "Phisiland";


int selectedCodeBase = 0;
WebServer DovetailSystem::server = {80};

void DovetailSystem::kickUser(uint8_t aid) {
    // Sends a deauthentication frame to the specific device
    esp_err_t err = esp_wifi_deauth_sta(aid);

    if (err == ESP_OK) {
        Serial.println("Deauthentication packet sent successfully.");
    } else {
        Serial.printf("Error kicking user: %s\n", esp_err_to_name(err));
    }
}

void DovetailSystem::wifiEvent(WiFiEvent_t event, arduino_event_info_t info) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED: {
            char macStr[18]; // Buffer to hold "XX:XX:XX:XX:XX:XX"
            uint8_t *mac = info.wifi_ap_staconnected.mac;
            uint16_t aid = info.wifi_ap_staconnected.aid;
            // Properly format the MAC address into a human-readable Hex string
            sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
                    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

            Serial.print("New client connected. MAC: ");
            Serial.println(macStr);

            if (Store::allowedMacs.count(macStr) <= 0) {
                Serial.println("Target device detected!");
                // WiFi.softAPdisconnect(false);
                kickUser(aid);
            }
            break;
        }

        case ARDUINO_EVENT_WIFI_STA_GOT_IP: {
            // For the ESP32's own MAC when it gets an IP
            Serial.print("ESP32 Station MAC: ");
            Serial.println(WiFi.macAddress());
            break;
        }
    }
}

void DovetailSystem::handleCode() {
    const String message = Store::codebase[selectedCodeBase];
    server.send(200, "text/plain", message);
}

void DovetailSystem::handlePin() {
    String message = "Responded!";
    server.send(200, "text/plain", message);
}


void DovetailSystem::init() {
    WiFi.softAP(ssid, password);
    WiFi.onEvent(wifiEvent);
    server.on("/code", handleCode);
    server.begin();
}
