//
// Created by Ezra Golombek on 11/03/2026.
//

#include "DovetailSystem.h"
#include <HTTPClient.h>
#include <DNSServer.h>
#include <esp_wifi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <map>
#include <SD.h>

#include "game/Game.h"
#include "hoist/HoistSystem.h"
#include "store/Store.h"
#include "ui_output/ui_FileSelection.h"
#include "widgets/label/lv_label.h"

std::vector<String> DovetailSystem::registeredMacsToVerify{};


std::map<String, IPAddress> DovetailSystem::macToIp;

std::map<String, String> DovetailSystem::macToName;

std::map<String, String> DovetailSystem::nameToMac;


std::map<String, String> DovetailSystem::macToCode;


#include <ArduinoJson.h>
#include <SD.h>
bool DovetailSystem::needsSave = false;
#include <ArduinoJson.h>
#include <SD.h>

String DovetailSystem::getCodeBaseForId(const String &id) {
    const auto mac = nameToMac[id];
    const auto codeBase = macToCode[mac];
    return codeBase;
}


DNSServer DovetailSystem::dnsServer;


AsyncWebServer DovetailSystem::server = {80};
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

// 1. Define a struct to pass multiple pieces of data to the task
struct MessageData {
    String ip;
    String message;
};

// 2. This is the background worker
QueueHandle_t msgQueue;

void messageWorker(void *pvParameters) {
    while (true) {
        MessageData *data;
        // Wait forever until a message arrives in the queue
        if (xQueueReceive(msgQueue, &data, portMAX_DELAY)) {
            HTTPClient http;
            String url = "http://" + data->ip + "/" + data->message;
            http.setTimeout(1500); // Keep it snappy

            if (http.begin(url)) {
                int httpCode = http.GET();
                Serial.printf("[HTTP] %s code: %d\n", data->ip.c_str(), httpCode);
                http.end();
            }

            delete data; // Clean up the struct
            vTaskDelay(pdMS_TO_TICKS(50)); // Tiny breather for the WiFi stack
        }
    }
}

void DovetailSystem::sendMessageToClient(const String &clientId, const String &message) {
    sendMessage(nameToMac[clientId], message);
}

// 3. The new non-blocking sendMessage
void DovetailSystem::sendMessage(const String &mac, const String &message) {
    if (macToIp.count(mac) == 0) {
        Serial.println("Error: MAC: " + mac + " not found in IP map.");
        return;
    }

    // Create a new data object on the heap

    // Launch background task
    // Stack size 4096 is usually safe for HTTPClient
    auto *data = new MessageData{macToIp[mac].toString(), message};
    if (xQueueSend(msgQueue, &data, 0) != pdPASS) {
        Serial.println("Queue full! Dropping message to save memory.");
        delete data;
    }
    Serial.println("Message queued in background for " + mac);
}


QueueHandle_t sdQueue;
// Background Task for SD Writing
void sdWorkerTask(void *pvParameters) {
    FileWritePacket packet{};
    for (;;) {
        // Wait indefinitely for a packet from the queue
        if (xQueueReceive(sdQueue, &packet, portMAX_DELAY)) {
            File f = SD.open(packet.path, packet.isFirst ? FILE_WRITE : FILE_APPEND);
            if (f) {
                f.write(packet.data, packet.len);
                f.close();
            }
            // CRITICAL: Free the memory we allocated in the callback
            free(packet.data);
        }
    }
}


void DovetailSystem::defineRoutes() {
    server.on("/code", HTTP_GET, [](AsyncWebServerRequest *request) {
        // String path = getLastRun(); // Your predefined file path
        if (!request->hasParam("mac")) {
            request->send(500, "text/plain", "Mac address not provided!");

            return;
        }
        String path = "/scripts/";
        if (macToCode.count(request->getParam("mac")->value())) {
            path += macToCode[request->getParam("mac")->value()];
        } else {
            path += "waterslide.ezra";
        }
        Serial.println(path);
        if (SD.exists(path)) {
            // Send the file. "text/plain" is usually best for code/scripts
            request->send(SD, path, "text/plain");
        } else {
            // Fallback if the file hasn't been created yet
            request->send(404, "text/plain", "Predefined file not found on SD");
        }
    });

    server.on("/sendResult", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("val") && request->hasParam("slot")) {
            String val = request->getParam("val")->value();
            String slot = request->getParam("slot")->value();
            // Sanitize the slot input to ensure it's only a, b, or c
            if (slot == "a" || slot == "b" || slot == "c") {
                if (slot == "a") {
                    Game::aValue = val;
                }
                if (slot == "b") {
                    Game::bValue = val;
                }
                if (slot == "c") {
                    Game::cValue = val;
                }
                Game::shouldUpdateValues = true;

                request->send(200, "text/plain", "Saved " + val + " to slot " + slot);
            } else {
                request->send(400, "text/plain", "Invalid slot. Use a, b, or c.");
            }
        } else {
            request->send(400, "text/plain", "Missing parameters. Need val and slot.");
        }
    });

    server.on("/screen", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (HoistSystem::inSetup) {
            Serial.println("Change screen recieved, however we are currently in the process of deploying!");
            return;
        }
        if (request->hasParam("id")) {
            String screenId = request->getParam("id")->value();

            Serial.println("Changing screen to: " + screenId);
            Game::screen = screenId;
            Game::shouldSwitchScreen = true;
        } else {
            request->send(400, "text/plain", "Missing parameters. Need val and slot.");
        }
    });
    server.on("/endActivity", HTTP_GET, [](AsyncWebServerRequest *request) {
        Serial.println("Ending activity");
        Game::shouldEndActivity = true;
        request->send(200, "text/plain", "Stopped the activity!");
    });


    server.on("/register", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("mac")) {
            request->send(400, "text/plain", "Missing mac address for registration!");
            return;
        }
        const auto mac = request->getParam("mac")->value();
        registeredMacsToVerify.push_back(mac);


        request->send(200, "text/plain", "Registered IP!");
    });
}

void DovetailSystem::connectionLoop() {
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
    initWifiName();
    WiFi.softAP(ssid, password);
    WiFi.setSleep(false);
    WiFi.onEvent(wifiEvent);
    // esp_wifi_set_inactive_time(WIFI_IF_AP, 10);
    sdQueue = xQueueCreate(60, sizeof(FileWritePacket));
    loadRegistryFromSD();
    xTaskCreatePinnedToCore(sdWorkerTask, "sdWorker", 4096, nullptr, 1, nullptr, 0);
    msgQueue = xQueueCreate(10, sizeof(MessageData *));

    // Start ONE worker task
    xTaskCreate(messageWorker, "MsgWorker", 4096, NULL, 1, NULL);


    if (!SD.exists("/scripts")) {
        SD.mkdir("/scripts");
    }
    defineRoutes();
    dnsServer.start(53, "am.it", WiFi.softAPIP());

    server.begin();
}

void DovetailSystem::resetAllDevices() {
    for (auto &name_to_mac: nameToMac) {
        sendMessage(name_to_mac.second, "reset");
    }
}

void DovetailSystem::connection() {
    connectMode = !connectMode;
}
