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
#include "store/FileServer.h"
#include "store/SDLock.h"
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
        case WS_EVT_DATA:
            data[len] = 0; // Null-terminate incoming character payload
            onWebSocketMessage(client->id(), static_cast<AwsFrameInfo *>(arg), reinterpret_cast<char *>(data), len);
            break;
        case WS_EVT_CONNECT:
        case WS_EVT_DISCONNECT:
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
        default: break;
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
    const auto mac = WifiModule::parsePrettyMac(request->getParam("mac")->value());
    const auto scriptFile = Store::getScriptFilePathByMac(mac);

    SDLock lock;
    request->send(SD, scriptFile, "text/plain");
}

void DovetailSystem::defineRoutes() {
    server.on("/code", HTTP_GET, code);
    server.onNotFound(notFound404);
}

void DovetailSystem::macVerificationLoop() {
    while (!Store::registeredMacsToVerify.empty()) {
        auto &[clientId,mac] = *Store::registeredMacsToVerify.begin();
        JsonDocument doc;
        const auto isAllowedOnNetwork = Store::macToCode.find(mac) != Store::macToCode.end();
        doc["command"] = isAllowedOnNetwork ? "register_success" : "register_failure";
        Logger::log(
            String("Mac: ") +
            WifiModule::macToString(mac).c_str() +
            (isAllowedOnNetwork
                 ? " is on the allowlist!"
                 : " was kicked off since they are not on the allowlist!"
            ));


        String messageContentToSend;
        serializeJson(doc, messageContentToSend);

        if (AsyncWebSocketClient *client = ws.client(clientId); client && client->status() == WS_CONNECTED)
            client->text(messageContentToSend);
        if (isAllowedOnNetwork)
            Store::registeredDeviceMacToClientId[mac] = clientId;
        Store::registeredMacsToVerify
                .
                erase(Store::registeredMacsToVerify.begin());
    }
}

void DovetailSystem::sendMessage(const std::array<uint8_t, 6> &mac, const String &message) {
    JsonDocument doc;
    doc["command"] = message;
    String serialized;
    serializeJson(doc, serialized);
    Logger::log("For testing purposes we are sending the message as a command! THIS NEEDS TO BE REFINED!");

    AsyncWebSocketClient *client = ws.client(Store::registeredDeviceMacToClientId[mac]);
    if (client && client->status() == WS_CONNECTED) {
        client->text(serialized);
    }
}

void DovetailSystem::sendMessage(const String &name, const String &message) {
    sendMessage(Store::nameToMac[name], message);
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
