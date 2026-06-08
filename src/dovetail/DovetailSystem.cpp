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

#include "DovetailEditor.h"
#include "WifiModule.h"
#include "game/Game.h"
#include "hoist/HoistSystem.h"
#include "store/Store.h"

String DovetailSystem::getCodeBaseForId(const String &id) {
    const auto mac = Store::nameToMac[id];
    const auto codeBase = Store::macToCode[mac];
    return codeBase;
}


AsyncDNSServer DovetailSystem::dnsServer;

AsyncWebServer DovetailSystem::server = {80};
AsyncWebSocket ws("/ws");


bool DovetailSystem::connectMode = false;


void onWebSocketMessage(const AwsFrameInfo *info, String message, size_t len) {
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        ws.textAll("test");
    }
}


void onWebSocketEvent(
    AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
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
            data[len] = 0; // Null-terminate incoming character payload
            onWebSocketMessage(static_cast<AwsFrameInfo *>(arg), reinterpret_cast<char *>(data), len);
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

void DovetailSystem::code(AsyncWebServerRequest *request) {
    if (!request->hasParam("mac")) {
        request->send(500, "text/plain", "Mac address not provided!");
        return;
    }


    const auto path = Store::getScriptFilePathByMac(request->getParam("mac")->value());

    if (SD.exists(path)) {
        // Send the file. "text/plain" is usually best for code/scripts
        request->send(SD, path, "text/plain");
    } else {
        // Fallback if the file hasn't been created yet
        request->send(404, "text/plain", "File not found on SD card!");
    }
}

void DovetailSystem::defineRoutes() {
    server.on("/code", HTTP_GET, code);
}

void DovetailSystem::connectionLoop() {
    /**
     * Hey buddy. U refactored and changed a lot of shit and forgot u did so. this is part of it. please remember that this used to be the logic that kicked users off the network when they joined.
     * You decided that you want to change that, and kick later.
     */
    // const auto ip = request->client()->remoteIP();
    // macToIp[mac] = request->client()->remoteIP();
    // Serial.println("Registered mac to ip");
    // if (!macToCode.count(mac)) {
    //     auto device = HoistSystem::currentHoistInDeployment.devices[HoistSystem::deviceIndex];
    //     macToCode[mac] = HoistSystem::inSetup ? device.file : "waterslide.ezra";
    //     macToName[mac] = HoistSystem::inSetup ? device.deviceId : mac;
    //     nameToMac[HoistSystem::inSetup ? device.deviceId : mac] = mac;
    //     needsSave = true;
    //     if (HoistSystem::status != 0) {
    //         HoistSystem::status = 2;
    //     }
    // }
    //
    // if (HoistSystem::inSetup) {
    //     Store::allowedMacs.insert(macStr);
    //     Store::saveToMacList();
    //     HoistSystem::status = 1;
    //     updateDeviceCount();
    //     break;
    // }
    //
    // if (!connectMode) {
    //     Serial.println("This client is not associated with this device!");
    //     kickUserWithMac(TODO);
    //     break;
    // }
}

void DovetailSystem::init() {
    defineRoutes();
    DovetailEditor::initEditorRoutes();
    WifiModule::startWifi();
}

void DovetailSystem::resetAllDevices() {
    for (auto &name_to_mac: Store::nameToMac) {
        // sendMessage(name_to_mac.second, "reset");
    }
}

void DovetailSystem::connection() {
    connectMode = !connectMode;
}
