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
    auto server = DovetailSystem::server;
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SD, "/index.html", "text/html");
    });
    server.serveStatic("/lib", SD, "/lib/");

    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
                   // Header response handled automatically or on completion
               }, nullptr, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
                   String fileName = "untitled.ezra";
                   if (request->hasParam("name", false)) {
                       fileName = request->getParam("name", false);
                   }

                   FileWritePacket packet{};
                   snprintf(packet.path, sizeof(packet.path), "/scripts/%s", fileName.c_str());
                   packet.len = len;
                   packet.isFirst = (index == 0);

                   // 3. Allocate a copy of the data.
                   // The 'data' pointer from AsyncWebServer is temporary and will be gone
                   // when this callback returns. We must make a persistent copy.
                   packet.data = (uint8_t *) malloc(len);
                   if (packet.data) {
                       memcpy(packet.data, data, len);

                       // 4. Push to queue. If queue is full, wait up to 100ms.
                       if (xQueueSend(Store::sdQueue, &packet, pdMS_TO_TICKS(100)) != pdPASS) {
                           free(packet.data); // Clean up if queue failed
                           Serial.println("SD Queue Full!");
                       }
                   }

                   if (index + len == total) {
                       request->send(200, "text/plain", "File queued for saving");
                   }
               });
    // Ensure this matches your JS call
    server.on("/read", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("name")) {
            String filename = request->getParam("name");
            String path = "/scripts/" + filename; // Must match the folder in /list

            if (SD.exists(path)) {
                request->send(SD, path, "text/plain");
            } else {
                request->send(404, "text/plain", "File Not Found");
            }
        }
    });
    server.on("/list-devices", HTTP_GET, [](AsyncWebServerRequest *request) {
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
    });
    server.on("/rename-device", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("mac") && request->hasParam("name")) {
            String mac = request->getParam("mac");
            String name = request->getParam("name");
            Store::macToName[mac] = name;
            Store::nameToMac[name] = mac;
            Store::needsSave = true;
            request->send(200, "text/plain", "Renamed");
        }
    });
    // 1. DELETE FILE
    server.on("/delete", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("name")) {
            String path = "/scripts/" + request->getParam("name");
            if (SD.remove(path)) request->send(200, "text/plain", "Deleted");
            else request->send(500, "text/plain", "Delete Failed");
        }
    });

    // 2. RENAME FILE
    server.on("/rename", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("old") && request->hasParam("new")) {
            String oldPath = "/scripts/" + request->getParam("old");
            String newPath = "/scripts/" + request->getParam("new");
            if (SD.rename(oldPath, newPath)) request->send(200, "text/plain", "Renamed");
            else request->send(500, "text/plain", "Rename Failed");
        }
    });

    // UPDATED LIST ROUTE (Only shows files in /scripts)
    server.on("/list", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = "[";
        File root = SD.open("/scripts");
        File file = root.openNextFile();
        while (file) {
            if (!file.isDirectory()) {
                if (json != "[") json += ",";
                json += "\"" + String(file.name()) + "\"";
            }
            file = root.openNextFile();
            yield();
        }
        json += "]";
        request->send(200, "application/json", json);
        root.close();
    });
    server.on("/run", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("name") && request->hasParam("mac")) {
            String filename = request->getParam("name");
            String deviceId = request->getParam("mac"); // This is the MAC

            // 1. Save locally so the Master knows what it last deployed
            Serial.println(
                "Running for device '" + deviceId + "' with " + String(Store::macToIp.count(deviceId)));

            // 2. Lookup the IP for this specific device
            // If you are using a std::map<String, IPAddress> macToIp:
            if (Store::macToIp.count(deviceId)) {
                // IPAddress targetIP = macToIp[deviceId];

                // 3. Send the message to the specific device
                // Assuming sendMessage handles the IP routing
                DovetailSystem::sendMessage(deviceId, "reset");
                Store::macToCode[deviceId] = filename;
                Store::needsSave = true;
                request->send(200, "text/plain", "Deploying " + filename + " to " + deviceId);
            } else {
                request->send(404, "text/plain", "Device not found or offline");
            }
        }
    });
}
