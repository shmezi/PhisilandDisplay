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
#include "game/Game.h"
#include "store/Store.h"


void DovetailEditor::initEditorRoutes() {
    auto &server = DovetailSystem::server;
    server.on("/debug", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", "WebServer Is Functional!");
    });
    server.on("/", HTTP_GET,  webpage);
    // server.serveStatic("/lib", SD, "/lib/");

    server.on("/save", HTTP_POST, saveFile);
    // Ensure this matches your JS call
    server.on("/read", HTTP_GET, readFile);
    server.on("/list-devices", HTTP_GET, listDevices);
    server.on("/rename-device", HTTP_GET, renameDevice);
    // 1. DELETE FILE
    server.on("/delete", HTTP_GET, deleteDevice);

    // 2. RENAME FILE
    server.on("/rename", HTTP_GET, renameFile);

    // UPDATED LIST ROUTE (Only shows files in /scripts)
    server.on("/list", HTTP_GET, listFiles);
    server.on("/run", HTTP_GET, runFile);
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
        device["mac"] = mac;
        device["ip"] = ip;
        device["name"] = Store::macToName[mac]; // Use MAC as name if empty
    }

    String output;
    serializeJson(responseDoc, output);
    request->send(200, "application/json", output);
}

void DovetailEditor::renameDevice(AsyncWebServerRequest *request) {
    if (request->hasParam("mac") && request->hasParam("name")) {
        String mac = request->getParam("mac")->value();
        String name = request->getParam("name")->value();
        Store::macToName[mac] = name;
        Store::nameToMac[name] = mac;
        Store::needsSave = true;
        request->send(200, "text/plain", "Renamed");
    }
}

void DovetailEditor::deleteDevice(AsyncWebServerRequest *request) {
    if (request->hasParam("name")) {
        String path = "/scripts/" + request->getParam("name")->value();
        if (SD.remove(path)) request->send(200, "text/plain", "Deleted");
        else request->send(500, "text/plain", "Delete Failed");
    }
}

void DovetailEditor::listFiles(AsyncWebServerRequest *request) {
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
    request->send(200, "application/json", json);

}

void DovetailEditor::runFile(AsyncWebServerRequest *request) {
    if (request->hasParam("name") && request->hasParam("mac")) {
        String filename = request->getParam("name")->value();
        String deviceId = request->getParam("mac")->value(); // This is the MAC

        // 1. Save locally so the Master knows what it last deployed
        Serial.println(
            "Running for device '" + deviceId + "' with " + String(Store::macToIp.count(deviceId)));

        // 2. Lookup the IP for this specific device
        // If you are using a std::map<String, IPAddress> macToIp:
        if (Store::macToIp.count(deviceId)) {
            // IPAddress targetIP = macToIp[deviceId];

            // 3. Send the message to the specific device
            // Assuming sendMessage handles the IP routing
            // DovetailSystem::sendMessage(deviceId, "reset"); TODO: RESET
            Store::macToCode[deviceId] = filename;
            Store::needsSave = true;
            request->send(200, "text/plain", "Deploying " + filename + " to " + deviceId);
        } else {
            request->send(404, "text/plain", "Device not found or offline");
        }
    }
}

void DovetailEditor::renameFile(AsyncWebServerRequest *request) {
    if (request->hasParam("old") && request->hasParam("new")) {
        String oldPath = "/scripts/" + request->getParam("old")->value();
        String newPath = "/scripts/" + request->getParam("new")->value();
        if (SD.rename(oldPath, newPath)) request->send(200, "text/plain", "Renamed");
        else request->send(500, "text/plain", "Rename Failed");
    }
}

void DovetailEditor::readFile(AsyncWebServerRequest *request) {
    if (request->hasParam("name")) {
        String filename = request->getParam("name")->value();
        String path = "/scripts/" + filename; // Must match the folder in /list

        if (SD.exists(path)) {
            request->send(SD, path, "text/plain");
        } else {
            request->send(404, "text/plain", "File Not Found");
        }
    }
}

void DovetailEditor::saveFile(AsyncWebServerRequest *request) {
    //REIMPlement
}

void DovetailEditor::webpage(AsyncWebServerRequest *request) {
    request->send(SD, "/phistudio.html", "text/html");


}
