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
String DovetailSystem::lastRunFile;
String DovetailSystem::ssid = "Dovetail-ERROR-SD-NOT-INSERTED!";

const char *password = "Phisiland";


// A map to store: Domain Name -> Client IP
std::map<String, IPAddress> customDomains;
DNSServer DovetailSystem::dnsServer;


int selectedCodeBase = 0;
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

void DovetailSystem::sendMessage(const String &message) {
    HTTPClient http;

    String serverPath = "http://" + customDomains["core.it"].toString() + "/" + message;

    // Your Domain name with URL path or IP address with path
    http.begin(serverPath.c_str());

    // If you need Node-RED/server authentication, insert user and password below
    //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");

    // Send HTTP GET request
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);
    } else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();
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
    const int rand1 = random(0, 30);
    const int rand2 = random(0, 30);

    const String newWifiName = "Dovetail-" + String(verbs[rand1]) + "-" + String(nouns[rand2]);
    if (f) {
        f.print(newWifiName);
        f.close();
        Serial.println("Stored wifi name: " + newWifiName);
        ssid = newWifiName;
    }
}

void DovetailSystem::saveLastRun(const String &filename) {
    File f = SD.open("/last_run.txt", FILE_WRITE);
    if (f) {
        f.print(filename);
        f.close();
        Serial.println("Stored last run: " + filename);
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

String getLastRun() {
    if (SD.exists("/last_run.txt")) {
        File f = SD.open("/last_run.txt", FILE_READ);
        if (f) {
            String lastFile = f.readString();
            f.close();

            // debug::log("Resuming last script: " + lastFile);

            // Re-use your existing read logic to get the code and run it
            String path = "/scripts/" + lastFile;
            if (SD.exists(path)) {
                return path;
            }
        }
    }

    return "Not found!";
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
        }
        json += "]";
        request->send(200, "application/json", json);
        root.close();
    });
    // 4. SEND PREDEFINED FILE
    server.on("/code", HTTP_GET, [](AsyncWebServerRequest *request) {
        // String path = getLastRun(); // Your predefined file path
        String path = lastRunFile;
        if (SD.exists(path)) {
            // Send the file. "text/plain" is usually best for code/scripts
            request->send(SD, path, "text/plain");
        } else {
            // Fallback if the file hasn't been created yet
            request->send(404, "text/plain", "Predefined file not found on SD");
        }
    });
    server.on("/run", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("name")) {
            String filename = request->getParam("name")->value();

            // 1. Save this name for next time
            saveLastRun(filename);

            sendMessage("reset");
            request->send(200, "text/plain", "Running and Saved: " + filename);
        }
    });
    server.on("/sendResult", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("val") && request->hasParam("slot")) {
            String val = request->getParam("val")->value();
            String slot = request->getParam("slot")->value();
            Serial.println("Result recieved: " + val + " " + slot);
            // Sanitize the slot input to ensure it's only a, b, or c
            if (slot == "a" || slot == "b" || slot == "c") {
                if (slot == "a") {
                    Game::setA(val);
                }
                if (slot == "b") {
                    Game::setB(val);
                }
                if (slot == "c") {
                    Game::setC(val);
                }

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
            Game::setCurrentScreen();
        } else {
            request->send(400, "text/plain", "Missing parameters. Need val and slot.");
        }
    });
    server.on("/register", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("domain")) {
            String domain = request->getParam("domain")->value();
            IPAddress clientIP = request->client()->remoteIP();

            customDomains[domain] = clientIP;

            // We tell the DNS server to start resolving this specific domain
            dnsServer.start(53, domain, clientIP);
            Serial.println("Registered ip of:" + clientIP.toString() + " as " + domain);
            request->send(200, "text/plain", "Registered " + domain + " to " + clientIP.toString());
        } else {
            request->send(400, "text/plain", "Missing domain param");
        }
    });
}

void DovetailSystem::init() {
    initWifiName();
    WiFi.softAP(ssid, password);
    WiFi.setSleep(false);
    WiFi.onEvent(wifiEvent);
    esp_wifi_set_inactive_time(WIFI_IF_AP, 10);
    sdQueue = xQueueCreate(10, sizeof(FileWritePacket));

    xTaskCreatePinnedToCore(sdWorkerTask, "sdWorker", 4096, nullptr, 1, nullptr, 0);

    if (!SD.exists("/scripts")) {
        SD.mkdir("/scripts");
    }
    defineRoutes();
    dnsServer.start(53, "am.it", WiFi.softAPIP());

    lastRunFile = getLastRun();
    server.begin();
}

void DovetailSystem::connection() {
    connectMode = !connectMode;
}
