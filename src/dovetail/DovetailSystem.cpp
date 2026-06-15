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
#include "WSCommandHandler.h"
#include "devices/DeviceManager.h"
#include "logging/Logger.h"
#include "logging/Logger.h"
#include "store/SDLock.h"
#include "store/Store.h"

AsyncDNSServer DovetailSystem::dnsServer;

AsyncWebServer DovetailSystem::server = {80};
std::map<ClientId, u_int32_t> DovetailSystem::registeredMacsToVerify;

void DovetailSystem::verifyDevice(const ClientId id, const u_int32_t webSocketID) {
    registeredMacsToVerify[id] = webSocketID;
    DeviceManager::getInstance().onDeviceConnect();
}

bool DovetailSystem::isDeviceAwaitingRegistration(const ClientId id) {
    return registeredMacsToVerify[id];
}

AsyncWebSocketClient *DovetailSystem::getWSClientByMac(const ClientId mac) {
    AsyncWebSocketClient *client = ws.client(DeviceManager::getInstance().getWSClientByMac(mac));
    if (client && client->status() == WS_CONNECTED) {
        return client;
    }
    return nullptr;
}

AsyncWebSocket DovetailSystem::ws("/ws");


bool DovetailSystem::connectMode = false;


void onWebSocketMessage(const uint8_t &wsClientId, const AwsFrameInfo *info, const String &message, size_t len) {
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, message);
        if (err) {
            Logger::error("JSON parse failed: " + String(err.c_str()));
            return;
        }
        WSCommandHandler::onIncomingMessage(wsClientId, doc);
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
    const auto scriptFile = DeviceManager::getInstance().getScriptFilePathByMac(mac);

    SDLock lock;
    request->send(SD, scriptFile, "text/plain");
}

void DovetailSystem::defineRoutes() {
    server.on("/code", HTTP_GET, code);
    server.onNotFound(notFound404);
}

void DovetailSystem::macVerificationLoop() {
    while (!registeredMacsToVerify.empty()) {
        auto &[mac,clientId] = *registeredMacsToVerify.begin();
        JsonDocument doc;
        const auto isAllowedOnNetwork = DeviceManager::getInstance().onDeviceRegistration(mac);

        doc["command"] = isAllowedOnNetwork ? "register_success" : "register_failure";
        Logger::log(
            String("Mac: ") +
            WifiModule::macToString(mac).c_str() +
            (isAllowedOnNetwork
                 ? " is on the allowlist!"
                 : " was kicked off since they are not on the allowlist!"
            ));
        if (isAllowedOnNetwork)
            DeviceManager::getInstance().registerDevice(mac, clientId);


        HoistSystem::getInstance().onDeviceRegistration(mac);


        String messageContentToSend;
        serializeJson(doc, messageContentToSend);

        if (const auto client = getWSClientByMac(mac); client != nullptr)
            client->text(messageContentToSend);

        registeredMacsToVerify.erase(registeredMacsToVerify.begin());
    }
}


void DovetailSystem::init() {
    defineRoutes();
    WSCommandHandler::registerAllInternalCommands();
    ws.onEvent(onWebSocketEvent);

    DovetailEditor::initEditorRoutes();
    WifiModule::startWifi();
}

void DovetailSystem::resetAllDevices() {
    std::vector<ClientId> macs;
    Logger::log("Resting for " + String(DeviceManager::getInstance().getConnectedDevices().size()) + " Devices");
    for (auto &[mac, _]: DeviceManager::getInstance().getConnectedDevices()) {
        macs.push_back(mac);
    }
    for (const auto &mac: macs) {
        Logger::log(("Resetting for mac" + WifiModule::macToString(mac)).data());
        WSCommandHandler::sendCommand(mac, "script", [](auto _) {
        });
    }
}

void DovetailSystem::connection() {
    connectMode = !connectMode;
}
