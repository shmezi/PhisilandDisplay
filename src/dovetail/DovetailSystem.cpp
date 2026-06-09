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
#include "logging/Logger.h"
#include "store/Store.h"

String DovetailSystem::getCodeBaseForId(const String &id) {
    const auto mac = Store::nameToMac[id];
    const auto codeBase = Store::macToCode[mac];
    return codeBase;
}


AsyncDNSServer DovetailSystem::dnsServer;

AsyncWebServer DovetailSystem::server = {80};
AsyncWebSocket DovetailSystem::ws("/ws");


bool DovetailSystem::connectMode = false;


void onWebSocketMessage(const uint8_t &wsClientId, const AwsFrameInfo *info, const String &message, size_t len) {
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, message);
        if (err) {
            Serial.printf("JSON parse failed: %s\n", err.c_str());
            return;
        }
        if (doc["command"] == "register") {
            Logger::log("Mac attempted to register: " + String(doc["mac"]));
            Store::registeredMacsToVerify[wsClientId] = WifiModule::parsePrettyMac(doc["mac"]);
        }
    }
}


void onWebSocketEvent(
    AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
    void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT: {
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(),
                          client->remoteIP().toString().c_str());
            JsonDocument doc;
            doc["command"] = "SHUTDOWN";
            String output;
            serializeJson(doc, output);
            client->text(output);
        }
        break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            break;
        case WS_EVT_DATA:
            data[len] = 0; // Null-terminate incoming character payload

            onWebSocketMessage(client->id(), static_cast<AwsFrameInfo *>(arg), reinterpret_cast<char *>(data), len);
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

void DovetailSystem::notFound404(AsyncWebServerRequest *request) {
    Logger::log("Incoming request for: " + request->url());
    request->send(404, "text/plain", "Not found");
}

void DovetailSystem::code(AsyncWebServerRequest *request) {
    if (!request->hasParam("mac")) {
        request->send(500, "text/plain", "Mac address not provided!");
        return;
    }

    // if (SD.exists(path)) {
        // Send the file. "text/plain" is usually best for code/scripts
        request->send(SD, "/scripts/smallDevice.ezra", "text/plain");
    // } else {
    //     // Fallback if the file hasn't been created yet
    //     request->send(404, "text/plain", "File not found on SD card!");
    // }
}

void DovetailSystem::defineRoutes() {
    server.on("/code", HTTP_GET, code);
    server.onNotFound(notFound404);

}

void DovetailSystem::macVerificationLoop() {
    while (!Store::registeredMacsToVerify.empty()) {
        Logger::log("Verifying a mac!");
        auto &[clientId,mac] = *Store::registeredMacsToVerify.begin();

        if (const auto macToCode = Store::macToCode.find(mac); macToCode == Store::macToCode.end()) {
            JsonDocument doc;
            doc["command"] = "register_success";
            String output;
            serializeJson(doc, output);

            ws.client(clientId)->text(output);
            Store::registeredMacsToVerify.erase(Store::registeredMacsToVerify.begin());
            continue;
        }

        Store::registeredMacsToVerify.erase(Store::registeredMacsToVerify.begin());
    }
}

void DovetailSystem::init() {
    defineRoutes();
    ws.onEvent(onWebSocketEvent);

    DovetailEditor::initEditorRoutes();
    WifiModule::startWifi();
}

void DovetailSystem::resetAllDevices() {
    for (auto &[name,mac]: Store::nameToMac) {
        // sendMessage(name_to_mac.second, "reset");
    }
}

void DovetailSystem::connection() {
    connectMode = !connectMode;
}
