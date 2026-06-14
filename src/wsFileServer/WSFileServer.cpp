//
// WSFileServer.cpp
//

#include "WSFileServer.h"
#include "store/Store.h"
#include "store/SDLock.h"
#include "devices/DeviceManager.h"
#include "devices/ClientId.h"
#include "logging/Logger.h"
#include "dovetail/WifiModule.h"
#include "dovetail/WSCommandHandler.h"
#include <SD.h>
#include <ArduinoJson.h>

AsyncWebSocket *WSFileServer::socket = nullptr;

void WSFileServer::init(AsyncWebServer &server) {
    socket = new AsyncWebSocket("/editor");
    socket->onEvent(_onEvent);
    server.addHandler(socket);
}

static void sendJson(AsyncWebSocket *ws, uint32_t cid, JsonDocument &doc) {
    String out;
    serializeJson(doc, out);
    ws->text(cid, out);
}

static void sendError(AsyncWebSocket *ws, uint32_t cid, const char *message) {
    JsonDocument doc;
    doc["cmd"] = "error";
    doc["message"] = message;
    sendJson(ws, cid, doc);
}

void WSFileServer::_onEvent(AsyncWebSocket *ws, AsyncWebSocketClient *client,
                            AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Logger::log("Editor WS connected: " + String(client->id()));
        return;
    }
    if (type == WS_EVT_DISCONNECT) {
        Logger::log("Editor WS disconnected: " + String(client->id()));
        return;
    }
    if (type != WS_EVT_DATA) return;

    auto *info = static_cast<AwsFrameInfo *>(arg);
    if (!info->final || info->index != 0 || info->len != len) return;
    if (info->opcode != WS_TEXT) return;

    JsonDocument req;
    if (deserializeJson(req, data, len) != DeserializationError::Ok) {
        sendError(ws, client->id(), "Bad JSON");
        return;
    }

    const String cmd = req["cmd"] | "";
    const uint32_t cid = client->id();
    JsonDocument out;

    // ── list ─────────────────────────────────────────────────────────────────
    if (cmd == "list") {
        out["cmd"] = "list";
        JsonArray arr = out["files"].to<JsonArray>();
        {
            SDLock lock;
            File root = SD.open("/scripts");
            File f = root.openNextFile();
            while (f) {
                if (Store::isStandardFile(f)) (void) arr.add(f.name());
                f.close();
                f = root.openNextFile();
            }
            root.close();
        }
        sendJson(ws, cid, out);
        return;
    }

    // ── read ─────────────────────────────────────────────────────────────────
    if (cmd == "read") {
        const String name = req["name"] | "";
        if (name.isEmpty()) {
            sendError(ws, cid, "Missing name");
            return;
        }
        const String path = "/scripts/" + name;
        {
            SDLock lock;
            File f = SD.open(path.c_str(), FILE_READ);
            if (!f) {
                sendError(ws, cid, "Could not open file");
                return;
            }
            out["cmd"] = "read";
            out["name"] = name;
            out["content"] = f.readString();
            f.close();
        }
        sendJson(ws, cid, out);
        return;
    }

    // ── rename ───────────────────────────────────────────────────────────────
    if (cmd == "rename") {
        const String oldName = req["old"] | "";
        const String newName = req["new"] | "";
        if (oldName.isEmpty() || newName.isEmpty()) {
            sendError(ws, cid, "Missing old/new");
            return;
        }
        {
            SDLock lock;
            if (!SD.rename(("/scripts/" + oldName).c_str(), ("/scripts/" + newName).c_str())) {
                sendError(ws, cid, "Rename failed");
                return;
            }
        }
        out["cmd"] = "rename";
        out["old"] = oldName;
        out["new"] = newName;
        sendJson(ws, cid, out);
        return;
    }

    // ── delete ───────────────────────────────────────────────────────────────
    if (cmd == "delete") {
        const String name = req["name"] | "";
        if (name.isEmpty()) {
            sendError(ws, cid, "Missing name");
            return;
        }
        {
            SDLock lock;
            if (!SD.remove(("/scripts/" + name).c_str())) {
                sendError(ws, cid, "Delete failed");
                return;
            }
        }
        out["cmd"] = "delete";
        out["name"] = name;
        sendJson(ws, cid, out);
        return;
    }

    // ── list-devices ─────────────────────────────────────────────────────────
    if (cmd == "list-devices") {
        out["cmd"] = "list-devices";
        JsonArray arr = out["devices"].to<JsonArray>();
        for (auto &[mac, _]: DeviceManager::getInstance().getConnectedDevices()) {
            auto dev = arr.add<JsonObject>();
            dev["mac"] = WifiModule::macToString(mac);
            dev["name"] = DeviceManager::getInstance().getDeviceNameById(mac);
        }
        sendJson(ws, cid, out);
        return;
    }

    // ── rename-device ────────────────────────────────────────────────────────
    if (cmd == "rename-device") {
        const String mac = req["mac"] | "";
        const String name = req["name"] | "";
        if (mac.isEmpty() || name.isEmpty()) {
            sendError(ws, cid, "Missing mac/name");
            return;
        }
        ClientId id = WifiModule::parsePrettyMac(mac);
        DeviceManager::getInstance().renameDevice(id, name);
        xSemaphoreGive(Store::needsSave);
        out["cmd"] = "rename-device";
        out["mac"] = mac;
        out["name"] = name;
        sendJson(ws, cid, out);
        return;
    }

    // ── run ──────────────────────────────────────────────────────────────────
    if (cmd == "run") {
        const String name = req["name"] | "";
        const String mac = req["mac"] | "";
        if (name.isEmpty() || mac.isEmpty()) {
            sendError(ws, cid, "Missing name/mac");
            return;
        }
        ClientId deviceMac = WifiModule::parsePrettyMac(mac);
        if (!DeviceManager::getInstance().isRegistered(deviceMac)) {
            sendError(ws, cid, "Device not found");
            return;
        }
        DeviceManager::getInstance().assignCodeBaseToDevice(deviceMac, name);
        xSemaphoreGive(Store::needsSave);
        WSCommandHandler::sendCommand(deviceMac, "script", [](auto _) {
        });
        out["cmd"] = "run";
        out["status"] = "ok";
        sendJson(ws, cid, out);
        return;
    }

    sendError(ws, cid, ("Unknown command: " + cmd).c_str());
}
