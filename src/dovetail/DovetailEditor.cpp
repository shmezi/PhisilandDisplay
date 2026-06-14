//
// Created by Ezra Golombek on 04/06/2026.
//

#include "DovetailEditor.h"
#include "DovetailSystem.h"

#include <HTTPClient.h>
#include <DNSServer.h>
#include <esp_wifi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

#include "devices/ClientId.h"
#include <map>
#include <SD.h>

#include "WifiModule.h"
#include "game/Game.h"
#include "logging/Logger.h"
#include "store/Store.h"
#include "DovetailEditor.h"
#include <SD.h>

#include "WSCommandHandler.h"
#include "devices/DeviceManager.h"
#include "store/FileServer.h"
#include "store/SDLock.h"
std::vector<uint8_t> DovetailEditor::htmlBuffer;

bool DovetailEditor::cacheWebpageToRAM() {
    // 1. Open the target file from storage
    File file = SD.open("/phistudio.html.gz", FILE_READ);
    if (!file) {
        return false;
    }

    size_t fileSize = file.size();
    if (fileSize == 0) {
        file.close();
        return false;
    }

    htmlBuffer.resize(fileSize);

    size_t bytesRead = file.read(htmlBuffer.data(), fileSize);
    file.close();

    if (bytesRead != fileSize) {
        htmlBuffer.clear();
        return false;
    }

    return true;
}


void DovetailEditor::initEditorRoutes() {
    auto &server = DovetailSystem::server;
    server.on("/debug", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", "WebServer Is Functional!");
    });

    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(204);
    });
    server.on("/", HTTP_GET, webpage);
    // server.serveStatic("/lib", SD, "/lib/");
    server.on("/save", HTTP_POST, [](auto*v){}, nullptr, saveFile); // keep HTTP POST for saves
    // server.on("/list-devices", HTTP_GET, listDevices);
    // server.on("/rename-device", HTTP_GET, renameDevice);
    // server.on("/delete", HTTP_GET, deleteScript);
    // server.on("/read", HTTP_GET, readFile);
    //
    // server.on("/rename", HTTP_GET, renameScript);
    // server.on("/list", HTTP_GET, listScripts);
    // server.on("/run", HTTP_GET, runScript);
    // server.on("/save", HTTP_POST, [](auto *v) {
    // }, nullptr, saveFile);
}

void DovetailEditor::listDevices(AsyncWebServerRequest *request) {
    // Flatten the object into an array for the frontend
    JsonDocument responseDoc;
    JsonArray array = responseDoc.to<JsonArray>();

    for (auto &[mac,_]: DeviceManager::getInstance().getConnectedDevices()) {
        auto device = array.add<JsonObject>();
        device["mac"] = WifiModule::macToString(mac);
        device["name"] = DeviceManager::getInstance().getDeviceNameById(mac); // Use MAC as name if empty
    }

    auto *response = request->beginResponseStream("application/json");
    serializeJson(responseDoc, *response);
    request->send(response);
}

void DovetailEditor::renameDevice(AsyncWebServerRequest *request) {
    if (!request->hasParam("mac") || !request->hasParam("name")) {
        request->send(400, "text/plain", "No mac was provided!");
    }

    const ClientId mac = WifiModule::parsePrettyMac(request->getParam("mac")->value());
    const String name = request->getParam("name")->value();

    DeviceManager::getInstance().renameDevice(mac, name);
    xSemaphoreGive(Store::needsSave);
    request->send(200, "text/plain", "OK");
}

void DovetailEditor::deleteScript(AsyncWebServerRequest *request) {
    if (!request->hasParam("name")) {
        request->send(400, "text/plain", "Missing name param!");
        return;
    }

    const auto path = std::make_shared<std::string>(
        std::string("/scripts/") + request->getParam("name")->value().c_str());
    FileServer::    dispatch(request, [p = path](auto result) {
        auto toDelete = *p.get();
        if (!SD.remove(toDelete.c_str())) {
            result->sendError("Delete Operation Failed");
            return;
        }
        result->sendSuccess("Deleted");
    });
}

void DovetailEditor::listScripts(AsyncWebServerRequest *request) {
    FileServer::dispatch(request, [](auto response) {
        JsonDocument doc;
        JsonArray array = doc.to<JsonArray>();

        File root = SD.open("/scripts");
        File file = root.openNextFile();
        while (file) {
            if (Store::isStandardFile(file)) {
                (void) array.add(file.name());
            }
            file.close();
            file = root.openNextFile();
            yield();
        }
        root.close();

        if (doc.overflowed()) {
            response->sendError("Too many scripts");
            return;
        }

        String json;
        serializeJson(doc, json);
        response->sendSuccess(std::move(json));
    });
}

void DovetailEditor::runScript(AsyncWebServerRequest *request) {
    if (request->hasParam("name") && request->hasParam("mac")) {
        String filename = request->getParam("name")->value();
        const auto formattedMac = request->getParam("mac")->value();
        ClientId deviceMac = WifiModule::parsePrettyMac(formattedMac);

        Logger::log("Script has been sent to device with id '" + formattedMac + "'");

        if (DeviceManager::getInstance().isRegistered(deviceMac)) {
            DeviceManager::getInstance().assignCodeBaseToDevice(deviceMac, filename);
            xSemaphoreGive(Store::needsSave);
            WSCommandHandler::sendCommand(deviceMac, "script", [](auto _) {
            });
            request->send(200, "text/plain", "Deploying " + filename + " to " + formattedMac);
        } else {
            request->send(404, "text/plain", "Device not found or offline");
        }
    }
}

void DovetailEditor::renameScript(AsyncWebServerRequest *request) {
    if (!request->hasParam("old") && !request->hasParam("new")) {
        request->send(400, "text/plain", "Missing old & new params");
        return;
    }
    const auto oldPath = std::make_shared<std::string>(
        std::string("/scripts/") + request->getParam("old")->value().c_str());
    const auto newPath = std::make_shared<std::string>(
        std::string("/scripts/") + request->getParam("new")->value().c_str());

    FileServer::dispatch(request, [ o = oldPath, n = newPath](auto response) {
        const auto oldV = *o.get();
        const auto newV = *n.get();

        if (!SD.rename(oldV.c_str(), newV.c_str())) {
            response->sendError("Failed to rename file!");
            return;
        }
        response->sendSuccess("Renamed file!");
    });
}

void DovetailEditor::readFile(AsyncWebServerRequest *request) {
    if (!request->hasParam("name")) {
        request->send(400, "text/plain", "Param 'name' missing!");
        return;
    }

    const auto filename = request->getParam("name")->value();
    if (!filename) {
        request->send(400, "text/plain", "Param 'name' missing!");
        Logger::log("Tried to read a file with no paramater.");
        return;
    }


    const auto path = std::make_shared<std::string>("/scripts/" + std::string(filename.c_str()));
    SDLock lock;

    FileServer::dispatch(request, [v = path](auto result) {
        const auto a = *v.get();
        File file = SD.open(a.c_str(), FILE_READ);
        Logger::log("Opening file:  '" + String(a.c_str()) + "'!");
        if (!file) {
            result->sendError("Could not open / read file!");
            return;
        }
        result->sendSuccess(file.readString());
        file.close();
    });
}

void DovetailEditor::saveFile(AsyncWebServerRequest *request,
                              uint8_t *data, size_t len,
                              size_t index, size_t total) {
    if (!request->hasParam("name")) {
        request->send(400, "text/plain", "Missing name");
        return;
    }
    String name = request->getParam("name")->value();
    SDLock lock;


    String tmpPath = "/scripts/_tmp_" + name;
    String path = "/scripts/" + name;

    // First chunk — create/overwrite the temp file
    File f = SD.open(tmpPath, index == 0 ? FILE_WRITE : FILE_APPEND);
    if (!f) {
        request->send(500, "text/plain", "Failed to open temp file");
        return;
    }
    f.write(data, len);
    f.close();

    // Only finalize once ALL chunks have arrived
    if (index + len >= total) {
        // Remove old file, rename temp into place
        if (SD.exists(path)) SD.remove(path);

        if (SD.rename(tmpPath, path)) {
            request->send(200, "text/plain", "Saved");
        } else {
            SD.remove(tmpPath); // clean up
            request->send(500, "text/plain", "Finalize failed");
        }
    }
}


void DovetailEditor::webpage(AsyncWebServerRequest *request) {
    // Ensure our buffer isn't empty before serving
    if (!htmlBuffer.empty()) {
        // Serve straight out of internal RAM
        AsyncWebServerResponse *response = request->beginResponse(
            200,
            "text/html",
            htmlBuffer.data(),
            htmlBuffer.size()
        );

        response->addHeader("Content-Encoding", "gzip");
        response->addHeader("Connection", "close"); // ← add this

        request->send(response);
    } else {
        // Fallback error if initialization failed or file was missing at boot
        request->send(500, "text/plain", "Web Editor Cache Empty!");
    }
}
