//
// Created by Ezra Golombek on 11/03/2026.
//

#include "DovetailSystem.h"
#include <HTTPClient.h>
#include <DNSServer.h>
#include <esp_wifi.h>
#include <ESPAsyncWebServer.h>
#include <map>
#include <SD.h>

#include "game/Game.h"
#include "hoist/HoistSystem.h"
#include "store/Store.h"

String DovetailSystem::getCodeBaseForId(const String &id) {
    const auto mac = nameToMac[id];
    const auto codeBase = macToCode[mac];
    return codeBase;
}


DNSServer DovetailSystem::dnsServer;


AsyncWebServer DovetailSystem::server = {80};
AsyncWebSocket ws("/ws");

bool DovetailSystem::connectMode = false;

void DovetailSystem::kickUserWithMac(const String &macToEvict) {
    wifi_sta_list_t connectedClients;
    esp_wifi_ap_get_sta_list(&connectedClients);

    for (int i = 0; i < connectedClients.num; i++) {
        wifi_sta_info_t client = connectedClients.sta[i];

        char currentClientMac[18];

        sprintf(currentClientMac, "%02X:%02X:%02X:%02X:%02X:%02X",
                client.mac[0], client.mac[1], client.mac[2],
                client.mac[3], client.mac[4], client.mac[5]);

        uint16_t aidOfClientToKick = 0;
        if (macToEvict.equalsIgnoreCase(currentClientMac)) {
            esp_wifi_ap_get_sta_aid(client.mac, &aidOfClientToKick);
            esp_wifi_deauth_sta(aidOfClientToKick);
            Serial.printf("🛡️ Sweeper Task: Kicked intruder %s\n", currentClientMac);
            break;
        }
    }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
    auto *info = static_cast<AwsFrameInfo *>(arg);
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        data[len] = 0; // Null-terminate incoming character payload
        String message = reinterpret_cast<char *>(data);

        ws.textAll("test");
    }
}


void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(),
                          client->remoteIP().toString().c_str());
            client->text("Welcome Client!");
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            break;
        case WS_EVT_DATA:
            handleWebSocketMessage(arg, data, len);
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}


void DovetailSystem::defineRoutes() {
    server.on("/code", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("mac")) {
            request->send(500, "text/plain", "Mac address not provided!");

            return;
        }


        const auto path = Store::getScriptFilePathByMac(request->getParam("mac"));

        if (SD.exists(path)) {
            // Send the file. "text/plain" is usually best for code/scripts
            request->send(SD, path, "text/plain");
        } else {
            // Fallback if the file hasn't been created yet
            request->send(404, "text/plain", "File not found on SD card!");
        }
    });
}

void DovetailSystem::connectionLoop() {
    /**
     * Hey buddy. U refactored and changed a lot of shit and forgot u did so. this is part of it. please remember that this used to be the logic that kicked users off the network when they joined.
     * You decided that you want to change that, and kick later.
     */
    const auto ip = request->client()->remoteIP();
    macToIp[mac] = request->client()->remoteIP();
    Serial.println("Registered mac to ip");
    if (!macToCode.count(mac)) {
        auto device = HoistSystem::deployment.devices[HoistSystem::deviceIndex];
        macToCode[mac] = HoistSystem::inSetup ? device.file : "waterslide.ezra";
        macToName[mac] = HoistSystem::inSetup ? device.deviceId : mac;
        nameToMac[HoistSystem::inSetup ? device.deviceId : mac] = mac;
        needsSave = true;
        if (HoistSystem::hoistStatus != 0) {
            HoistSystem::hoistStatus = 2;
        }
    }

    if (HoistSystem::inSetup) {
        Store::allowedMacs.insert(macStr);
        Store::saveToMacList();
        HoistSystem::hoistStatus = 1;
        updateDeviceCount();
        break;
    }

    if (!connectMode) {
        Serial.println("This client is not associated with this device!");
        kickUserWithMac(TODO);
        break;
    }
}

void DovetailSystem::init() {
    defineRoutes();
}

void DovetailSystem::resetAllDevices() {
    for (auto &name_to_mac: nameToMac) {
        sendMessage(name_to_mac.second, "reset");
    }
}

void DovetailSystem::connection() {
    connectMode = !connectMode;
}
