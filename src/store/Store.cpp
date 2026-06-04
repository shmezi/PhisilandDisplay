//
// Created by Ezra Golombek on 11/03/2026.
//

#include "Store.h"
#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <set>

#include "dovetail/DovetailSystem.h"
#include "logging/Logger.h"


std::map<String, IPAddress> Store::macToIp;

std::map<String, String> Store::macToName;

std::map<String, String> Store::nameToMac;


std::map<String, String> Store::macToCode;

bool Store::needsSave = false;


void Store::initSD() {
    Logger::log("Initializing SD card...");
    SD.begin(5, SPI, 4000000, "/sd", 10);
    Logger::log("Card initialized.");
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

String readScriptFileToString(const String &path) {
    File script = SD.open("/" + path + ".ezra");
    if (!script) {
        Logger::error("Failed to open " + path + " script.");
        return "";
    }

    String fileContent = "";

    while (script.available()) {
        const char charRead = script.read();
        fileContent += charRead;
    }

    // Close the file
    script.close();

    return fileContent;
}

bool Store::ensureFileExists(const String &name) {
    if (SD.exists("/" + name)) return true;
    Logger::log(name + " not found. Initializing empty file.");
    File file = SD.open("/" + name, FILE_WRITE);
    if (!file) {
        Logger::error("Failed to open file for writing");
        return false;
    }
    file.close();
    return false;
}

bool Store::getFileOrCreateDefault(const String &name, const std::function<bool(File &file)> &defaultValue) {
    if (SD.exists("/" + name)) return true;
    Logger::log(name + " not found. Initializing empty file.");
    File file = SD.open("/" + name, FILE_WRITE);
    if (!file) {
        Logger::error("Failed to open file for writing");
        return false;
    }
    Logger::log("Running default operations on file.");
    const auto defaultConfigApplied = defaultValue(file);
    if (defaultConfigApplied)
        Logger::log("Default file applied successfully.");
    else
        Logger::error("Failed to apply default file!");

    file.close();
    return false;
}


void Store::loadRegistryFromSD() {
    getFileOrCreateDefault("config.json", [](File f) {
        saveRegistryToSD(f);
        return true;
    });


    File file = SD.open("/config.json", FILE_READ);
    if (file.size() == 0) {
        file.close();
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Logger::error("JSON Parse Error: " + String(error.c_str()));
        return;
    }

    // Clear current RAM maps to prevent data duplication/stale entries
    DovetailSystem::macToName.clear();
    DovetailSystem::macToCode.clear();

    JsonObject root = doc.as<JsonObject>();
    for (JsonPair p: root) {
        String mac = p.key().c_str();
        JsonObject data = p.value();

        DovetailSystem::macToCode[mac] = data["code"] | "default.ezra";


        // Build nameToMac Map (Name is Key, MAC is Value)
        String name = data["name"] | "NoName";
        DovetailSystem::macToName[mac] = name;
        DovetailSystem::nameToMac[name] = mac;
    }
    Logger::log("Loaded " + String(DovetailSystem::macToCode.size()) + " devices from SD.");
}

void Store::saveRegistryToSD(File &file) {
    if (!DovetailSystem::needsSave) return;
    JsonDocument doc;

    for (auto const &[mac, code]: DovetailSystem::macToCode) {
        doc[mac]["code"] = code;

        // Check if this MAC also has a nickname in nameToMac
        // We iterate through nameToMac to find the matching MAC
        for (auto const &[m, name]: DovetailSystem::macToName) {
            if (m == mac) {
                doc[mac]["name"] = name;
                break;
            }
        }
    }
    serializeJson(doc, file);

    DovetailSystem::needsSave = false;
}

String Store::getScriptFilePathByMac(const String &mac) {
    auto assignedScriptToMac = macToCode.find(mac);
    auto macHasAssignedCodebase = assignedScriptToMac != macToCode.end();
    return String("/scripts/") + (macHasAssignedCodebase ? assignedScriptToMac->second : "waterslide.ezra");
}
void Store::initValuesFromSD() {
    initSD();
    if (!SD.exists("/scripts")) {
        SD.mkdir("/scripts");
    }
    sdQueue = xQueueCreate(60, sizeof(FileWritePacket));

    xTaskCreatePinnedToCore(sdWorkerTask, "sdWorker", 4096, nullptr, 1, nullptr, 0);
}
