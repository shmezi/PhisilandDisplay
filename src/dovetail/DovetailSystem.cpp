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
#include "store/Store.h"
#include "ui_output/ui_FileSelection.h"
#include "widgets/label/lv_label.h"
const char *verbs[] = {
    "Carve",
    "Shape",
    "Cut",
    "Split",
    "Sand",
    "Plant",
    "Grow",
    "Climb",
    "Track",
    "Forge",
    "Bend",
    "Lash",
    "Stack",
    "Scout",
    "Tend"
};

const char *nouns[] = {
    "Wood",
    "Oak",
    "Pine",
    "Bark",
    "Roots",
    "Trail",
    "Grove",
    "Stone",
    "Fire",
    "Axe",
    "Camp",
    "Ridge",
    "Creek",
    "Fern",
    "Moss"
};
String DovetailSystem::ssid = "Dovetail-ERROR-SD-NOT-INSERTED!";

const char *password = "Phisiland";

std::map<String, IPAddress> macToIp;
std::map<String, String> macToName;
std::map<String, String> nameToMac;

std::map<String, String> macToCode;


#include <ArduinoJson.h>
#include <SD.h>
bool DovetailSystem::needsSave = false;
#include <ArduinoJson.h>
#include <SD.h>

void DovetailSystem::saveRegistryToSD() {
    if (!needsSave) return;
    JsonDocument doc;

    // We use macToCode as the primary loop because every registered
    // device should have a code assignment (even if it's "default.ezra")
    for (auto const &[mac, code]: macToCode) {
        doc[mac]["code"] = code;

        // Check if this MAC also has a nickname in nameToMac
        // We iterate through nameToMac to find the matching MAC
        for (auto const &[m, name]: macToName) {
            if (m == mac) {
                doc[mac]["name"] = name;
                break;
            }
        }
    }

    File file = SD.open("/config.json", FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open config for writing");
        return;
    }

    serializeJson(doc, file);
    file.close();
    needsSave = false;
    Serial.println("Phisiland config saved to SD.");
}

void DovetailSystem::loadRegistryFromSD() {
    if (!SD.exists("/config.json")) {
        Serial.println("ℹ️ config.json not found. Initializing empty file.");
        saveRegistryToSD();
        return;
    }

    File file = SD.open("/config.json", FILE_READ);
    if (file.size() == 0) {
        file.close();
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.printf("❌ JSON Parse Error: %s\n", error.c_str());
        return;
    }

    // Clear current RAM maps to prevent data duplication/stale entries
    macToName.clear();
    macToCode.clear();

    JsonObject root = doc.as<JsonObject>();
    for (JsonPair p: root) {
        String mac = p.key().c_str(); // MAC is the primary key in JSON
        JsonObject data = p.value();

        // Restore macToCode Map
        if (data.containsKey("code")) {
            macToCode[mac] = data["code"].as<String>();
        }

        // Restore nameToMac Map (Name is Key, MAC is Value)
        if (data.containsKey("name")) {
            String name = data["name"].as<String>();
            macToName[mac] = name;
            nameToMac[name] = mac;
        }
    }
    Serial.printf("✅ Successfully loaded %d devices from SD.\n", (int) macToCode.size());
}

DNSServer DovetailSystem::dnsServer;


AsyncWebServer DovetailSystem::server = {80};
bool DovetailSystem::connectMode = false;

void DovetailSystem::kickUser(uint8_t aid) {
    // Sends a deauthentication frame to the specific device
    esp_err_t err = esp_wifi_deauth_sta(aid);

    if (err == ESP_OK) {
        Serial.println("Deauthentication packet sent successfully.");
    } else {
        Serial.printf("Error kicking user: %s\n", esp_err_to_name(err));
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
    MessageData *data = new MessageData{macToIp[mac].toString(), message};
    if (xQueueSend(msgQueue, &data, 0) != pdPASS) {
        Serial.println("Queue full! Dropping message to save memory.");
        delete data;
    }
    Serial.println("Message queued in background for " + mac);
}

void DovetailSystem::initWifiName() {
    if (SD.exists("/wifi-name.txt")) {
        File f = SD.open("/wifi-name.txt", FILE_READ);
        if (f) {
            ssid = f.readString();
            f.close();
        }
        return;
    }
    File f = SD.open("/wifi-name.txt", FILE_WRITE);
    const int rand1 = random(0, 14);
    const int rand2 = random(0, 14);

    const String newWifiName = "Dovetail-" + String(verbs[rand1]) + "-" + String(nouns[rand2]);
    if (f) {
        f.print(newWifiName);
        f.close();
        Serial.println("Stored wifi name: " + newWifiName);
        ssid = newWifiName;
    }
}

void DovetailSystem::updateDeviceCount() {
    const uint8_t numStations = WiFi.softAPgetStationNum();
    String text = "Connected clients: ";
    text += String(numStations);
    lv_label_set_text(ui_ConnectedLabel, text.c_str());
}

void DovetailSystem::wifiEvent(WiFiEvent_t event, arduino_event_info_t info) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED: {
            char macStr[18];
            uint8_t *mac = info.wifi_ap_staconnected.mac;
            uint16_t aid = info.wifi_ap_staconnected.aid;
            // Properly format the MAC address into a human-readable Hex string
            sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
                    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

            Serial.print("Client connected. MAC: ");
            Serial.println(macStr);

            if (Store::allowedMacs.count(macStr) > 0) {
                Serial.println("Exists!");
                updateDeviceCount();

                break;
            }

            if (!connectMode) {
                Serial.println("This client is not associated with this device!");
                kickUser(aid);
                break;
            }
            updateDeviceCount();

            Store::allowedMacs.insert(macStr);
            Store::saveToMacList();
            break;
        }

        case ARDUINO_EVENT_WIFI_STA_GOT_IP: {
            // For the ESP32's own MAC when it gets an IP
            Serial.print("ESP32 Station MAC: ");
            Serial.println(WiFi.macAddress());
            break;
        }

        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED: {
            updateDeviceCount();
        }
        default:
            break;
    }
}

struct FileWritePacket {
    char path[64];
    uint8_t *data;
    size_t len;
    bool isFirst;
};

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
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SD, "/index.html", "text/html");
    });
    server.serveStatic("/lib", SD, "/lib/");

    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
                  // Header response handled automatically or on completion
              }, nullptr, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
                  String fileName = "untitled.ezra";
                  if (request->hasParam("name", false)) {
                      fileName = request->getParam("name", false)->value();
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
                      if (xQueueSend(sdQueue, &packet, pdMS_TO_TICKS(100)) != pdPASS) {
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
            String filename = request->getParam("name")->value();
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

        for (auto &v: macToIp) {
            auto &mac = v.first;
            auto &ip = v.second;
            JsonObject device = array.add<JsonObject>();
            device["mac"] = mac;
            device["ip"] = ip;
            device["name"] = macToName[mac]; // Use MAC as name if empty
        }

        String output;
        serializeJson(responseDoc, output);
        request->send(200, "application/json", output);
    });
    server.on("/rename-device", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("mac") && request->hasParam("name")) {
            String mac = request->getParam("mac")->value();
            String name = request->getParam("name")->value();
            macToName[mac] = name;
            nameToMac[name] = mac;
            needsSave = true;
            request->send(200, "text/plain", "Renamed");
        }
    });
    // 1. DELETE FILE
    server.on("/delete", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("name")) {
            String path = "/scripts/" + request->getParam("name")->value();
            if (SD.remove(path)) request->send(200, "text/plain", "Deleted");
            else request->send(500, "text/plain", "Delete Failed");
        }
    });

    // 2. RENAME FILE
    server.on("/rename", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("old") && request->hasParam("new")) {
            String oldPath = "/scripts/" + request->getParam("old")->value();
            String newPath = "/scripts/" + request->getParam("new")->value();
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
    // 4. SEND PREDEFINED FILE
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
    server.on("/run", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("name") && request->hasParam("mac")) {
            String filename = request->getParam("name")->value();
            String deviceId = request->getParam("mac")->value(); // This is the MAC

            // 1. Save locally so the Master knows what it last deployed
            Serial.println("Running for device '" + deviceId + "' with " + String(macToIp.count(deviceId)));

            // 2. Lookup the IP for this specific device
            // If you are using a std::map<String, IPAddress> macToIp:
            if (macToIp.count(deviceId)) {
                // IPAddress targetIP = macToIp[deviceId];

                // 3. Send the message to the specific device
                // Assuming sendMessage handles the IP routing
                sendMessage(deviceId, "reset");
                macToCode[deviceId] = filename;
                needsSave = true;
                request->send(200, "text/plain", "Deploying " + filename + " to " + deviceId);
            } else {
                request->send(404, "text/plain", "Device not found or offline");
            }
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
        macToIp[mac] = request->client()->remoteIP();
        Serial.println("Registered mac to ip");
        if (!macToCode.count(mac)) {
            macToCode[mac] = "waterslide.ezra";
            macToName[mac] = mac;
            nameToMac[mac] = mac;
            needsSave = true;
        }
        request->send(200, "text/plain", "Registered IP!");
    });
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

void DovetailSystem::connection() {
    connectMode = !connectMode;
}
