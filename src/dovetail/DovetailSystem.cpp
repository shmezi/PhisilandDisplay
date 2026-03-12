//
// Created by Ezra Golombek on 11/03/2026.
//

#include "DovetailSystem.h"
#include <HTTPClient.h>

#include <esp_wifi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <SD.h>

#include "store/Store.h"
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

int rand1 = random(0, 30);
int rand2 = random(0, 30);
String ssid = "Dovetail-Testing";

// String ssid = "Dovetail-" + String(verbs[rand1]) + "-" + String(nouns[rand2]);
const char *password = "Phisiland";


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

void DovetailSystem::sendMessage() {
    HTTPClient http;

    String serverPath = "http://192.168.4.2/reset";

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
                break;
            }

            if (!connectMode) {
                Serial.println("This client is not associated with this device!");
                kickUser(aid);
                break;
            }

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
    }
}


void DovetailSystem::init() {

    WiFi.softAP(ssid, password);
    WiFi.onEvent(wifiEvent);
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SD, "/index.html", "text/html");
    });
    server.serveStatic("/lib", SD, "/lib/");
    // Create the directory if it doesn't exist
    if (!SD.exists("/scripts")) {
        SD.mkdir("/scripts");
    }

    // UPDATED SAVE ROUTE
    // Expects a URL like: /save?name=myfile.phi
    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
              }, nullptr,
              [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
                  String fileName = "untitled.ezra";
                  if (request->hasParam("name")) {
                      fileName = request->getParam("name")->value();
                  }

                  // Ensure we save into the scripts folder
                  String path = "/scripts/" + fileName;

                  File f = SD.open(path, FILE_WRITE);
                  if (f) {
                      f.write(data, len);
                      f.close();
                      request->send(200, "text/plain", "Saved: " + path);
                  } else {
                      request->send(500, "text/plain", "SD Write Failed");
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
    server.on("/delete", HTTP_GET, [](AsyncWebServerRequest *request){
      if(request->hasParam("name")) {
        String path = "/scripts/" + request->getParam("name")->value();
        if(SD.remove(path)) request->send(200, "text/plain", "Deleted");
        else request->send(500, "text/plain", "Delete Failed");
      }
    });

    // 2. RENAME FILE
    server.on("/rename", HTTP_GET, [](AsyncWebServerRequest *request){
      if(request->hasParam("old") && request->hasParam("new")) {
        String oldPath = "/scripts/" + request->getParam("old")->value();
        String newPath = "/scripts/" + request->getParam("new")->value();
        if(SD.rename(oldPath, newPath)) request->send(200, "text/plain", "Renamed");
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
        String path = "/scripts/waterslide.code"; // Your predefined file path

        if (SD.exists(path)) {
            // Send the file. "text/plain" is usually best for code/scripts
            request->send(SD, path, "text/plain");
        } else {
            // Fallback if the file hasn't been created yet
            request->send(404, "text/plain", "Predefined file not found on SD");
        }
    });
    server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request) {
     sendMessage();
   });
    server.begin();
}


void DovetailSystem::connection() {
    connectMode = !connectMode;
}
