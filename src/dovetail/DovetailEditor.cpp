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


#include <map>
#include <SD.h>

#include "WifiModule.h"
#include "game/Game.h"
#include "logging/Logger.h"
#include "store/Store.h"
#include "DovetailEditor.h"
#include <SD.h>

#include "store/FileServer.h"
std::vector<uint8_t> DovetailEditor::htmlBuffer;

bool DovetailEditor::cacheWebpageToRAM() {
    // 1. Open the target file from storage
    File file = SD.open("/phistudio.html.gz", FILE_READ);
    if (!file) {
        // [LOG] Error: Failed to open webpage file for caching!
        return false;
    }

    size_t fileSize = file.size();
    if (fileSize == 0) {
        file.close();
        return false;
    }

    // 2. Resize our vector to fit the file perfectly (allocates RAM)
    htmlBuffer.resize(fileSize);

    // 3. Read the entire raw binary stream straight into RAM memory
    size_t bytesRead = file.read(htmlBuffer.data(), fileSize);
    file.close();

    // 4. Verify the transfer integrity
    if (bytesRead != fileSize) {
        htmlBuffer.clear(); // Free memory if reading failed midway
        return false;
    }

    // [LOG] Web Editor successfully cached to RAM! Size: X bytes.
    return true;
}


void DovetailEditor::initEditorRoutes() {
    auto &server = DovetailSystem::server;
    server.on("/debug", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", "WebServer Is Functional!");
    });
    server.on("/", HTTP_GET, webpage);
    // server.serveStatic("/lib", SD, "/lib/");

    server.on("/list-devices", HTTP_GET, listDevices);
    server.on("/rename-device", HTTP_GET, renameDevice);
    server.on("/delete", HTTP_GET, deleteScript);
    server.on("/read", HTTP_GET, readFile);

    server.on("/rename", HTTP_GET, renameScript);
    server.on("/list", HTTP_GET, listScripts);
    server.on("/run", HTTP_GET, runScript);
    server.on("/save", HTTP_POST, nullptr, nullptr, saveFile);
}

void DovetailEditor::listDevices(AsyncWebServerRequest *request) {
    JsonDocument doc;

    // Flatten the object into an array for the frontend
    JsonDocument responseDoc;
    JsonArray array = responseDoc.to<JsonArray>();

    for (auto &v: Store::macToIp) {
        auto &mac = v.first;
        auto &ip = v.second;
        JsonObject device = array.add<JsonObject>();
        device["mac"] = WifiModule::macToString(mac);
        device["ip"] = ip;
        device["name"] = Store::macToName[mac]; // Use MAC as name if empty
    }

    String output;
    serializeJson(responseDoc, output);
    request->send(200, "application/json", output);
}

void DovetailEditor::renameDevice(AsyncWebServerRequest *request) {
    if (request->hasParam("mac") && request->hasParam("name")) {
        std::array<uint8_t, 6> mac = WifiModule::parsePrettyMac(request->getParam("mac")->value());
        String name = request->getParam("name")->value();
        Store::macToName[mac] = name;
        Store::nameToMac[name] = mac;
        Store::needsSave = true;
        request->send(200, "text/plain", "Renamed");
    }
}

void DovetailEditor::deleteScript(AsyncWebServerRequest *request) {
    if (!request->hasParam("name")) {
        request->send(400, "text/plain", "Missing name param!");
        return;
    }

    const auto path = "/scripts/" + request->getParam("name")->value();
    FileServer::dispatch(request, [path](auto result) {
        if (!SD.remove(path)) {
            result->sendError("Delete Operation Failed");
            return;
        }
        result->sendSuccess("Deleted");
    });
}

void DovetailEditor::listScripts(AsyncWebServerRequest *request) {
    FileServer::dispatch(request, [](auto response) {
        String json = "[";
        File root = SD.open("/scripts");
        File file = root.openNextFile();
        while (file) {
            if (Store::isStandardFile(file)) {
                if (json != "[") json += ",";
                json += "\"" + String(file.name()) + "\"";
            }
            file = root.openNextFile();
            yield();
        }
        root.close();
        json += "]";
        response->sendSuccess(json);
    });
}

void DovetailEditor::runScript(AsyncWebServerRequest *request) {
    if (request->hasParam("name") && request->hasParam("mac")) {
        String filename = request->getParam("name")->value();
        const auto formattedMac = request->getParam("mac")->value();
        std::array<uint8_t, 6> deviceMac = WifiModule::parsePrettyMac(formattedMac);
        // This is the MAC

        // 1. Save locally so the Master knows what it last deployed
        Logger::log(
            "Running for device '" + formattedMac + "' with " + String(Store::macToIp.count(deviceMac)));

        // 2. Lookup the IP for this specific device
        // If you are using a std::map<String, IPAddress> macToIp:
        if (Store::macToIp.count(deviceMac)) {
            // IPAddress targetIP = macToIp[deviceId];

            // 3. Send the message to the specific device
            // Assuming sendMessage handles the IP routing
            // DovetailSystem::sendMessage(deviceId, "reset"); TODO: RESET
            Store::macToCode[deviceMac] = filename;
            Store::needsSave = true;
            request->send(200, "text/plain", "Deploying " + filename + " to " + formattedMac);
        } else {
            request->send(404, "text/plain", "Device not found or offline");
        }
    }
}

void DovetailEditor::renameScript(AsyncWebServerRequest *request) {
    if (!request->hasParam("old") && request->hasParam("new")) {
        request->send(400, "text/plain", "Missing old & new params");
        return;
    }
    const auto oldPath = "/scripts/" + request->getParam("old")->value();
    const auto newPath = "/scripts/" + request->getParam("new")->value();
    FileServer::dispatch(request, [oldPath,newPath](auto response) {
        if (!SD.rename(oldPath, newPath)) {
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

    FileServer::dispatch(request, [filename](auto result) {
        const auto path = "/scripts/" + filename;
        File file = SD.open(path.c_str(), FILE_READ);

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
