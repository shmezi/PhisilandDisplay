//
// Created by Ezra Golombek on 11/03/2026.
//

#include "Store.h"
#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <set>

#include "dovetail/DovetailSystem.h"
#include "dovetail/WifiModule.h"
#include "hoist/HoistSystem.h"
#include "logging/Logger.h"
#include "widgets/roller/lv_roller.h"


struct ClientConfig;
std::map<std::array<uint8_t, 6>, IPAddress> Store::macToIp;

std::map<std::array<uint8_t, 6>, String> Store::macToName;
std::vector<std::array<uint8_t, 6> > Store::registeredMacsToVerify;
std::map<String, std::array<uint8_t, 6> > Store::nameToMac;


std::map<std::array<uint8_t, 6>, String> Store::macToCode;
//Hey future confused me, Ezra from 9/06, I haven't checked but I think this needs save is regarding the config file.
//If it's wrong, just change it's name.. Ezra from the past really should have named stuff better aye?
bool Store::needsSave = false;


void Store::initSD() {
    Logger::log("Initializing SD card...");
    SD.begin(5, SPI, 4000000, "/sd", 10);
    Logger::log("Card initialized.");
}

QueueHandle_t Store::sdQueue;
// Background Task for SD Writing
void sdWorkerTask(void *pvParameters) {
    FileWritePacket packet{};
    for (;;) {
        // Wait indefinitely for a packet from the queue
        if (xQueueReceive(Store::sdQueue, &packet, portMAX_DELAY)) {
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
    if (name.indexOf('.') == -1) {
        SD.mkdir("/" + name);
        return true;
    }
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

/**
 * Check if the file passed to it is in fact a file, not a directory. And that it is not one of the internal mac hidden files.
 * @param file The file to check
 * @return The result of the check
 */
bool Store::isStandardFile(File &file) {
    return !(file.isDirectory() || String(file.name()).startsWith("._"));
}

String Store::hoistEntriesForHoistSelection = "";

void Store::registerHoistId(const String &id) {
    if (hoistEntriesForHoistSelection != "") hoistEntriesForHoistSelection += "\n";
    hoistEntriesForHoistSelection += id;
}

ClientConfig Store::loadClientFromVariant(const JsonVariant &clientDocument) {
    const auto device = clientDocument.as<JsonObject>();

    const auto deviceId = device["id"].as<String>();
    const auto deviceDescription = device["description"].as<String>();
    const auto deviceFile = device["file"].as<String>();
    return {deviceId, deviceDescription, deviceFile};
}

Hoist Store::loadHoistFromDocument(JsonDocument &hoistDocument) {
    const auto hoistID = hoistDocument["name"].as<String>();
    const auto description = hoistDocument["description"].as<String>();
    std::vector<ClientConfig> devices{};

    registerHoistId(hoistID);
    const auto deviceList = hoistDocument["devices"].as<JsonArray>();
    for (const auto &deviceDocument: deviceList) {
        devices.push_back(loadClientFromVariant(deviceDocument));
    }
    return {hoistID, description, devices};
}

void Store::loadHoists() {
    hoistEntriesForHoistSelection = "";
    auto hoistsDirectory = SD.open("/hoists");


    auto hoistFile = hoistsDirectory.openNextFile();
    while (hoistFile) {
        if (!isStandardFile(hoistFile)) {
            hoistFile = hoistsDirectory.openNextFile();
            continue;
        }

        JsonDocument hoistDocument;
        DeserializationError error = deserializeJson(hoistDocument, hoistFile);
        hoistFile.close();
        if (error) {
            Logger::error("Error reading hoist file: '" + String(hoistFile.name()) + "'!");
            continue;
        }
        const auto hoist = loadHoistFromDocument(hoistDocument);

        HoistSystem::hoists[hoist.id] = hoist;


        hoistFile = hoistsDirectory.openNextFile();
    }


    hoistsDirectory.close();
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
    macToName.clear();
    macToCode.clear();

    JsonObject root = doc.as<JsonObject>();
    for (JsonPair p: root) {
        String formattedMac = p.key().c_str();

        auto mac = WifiModule::parsePrettyMac(formattedMac);
        JsonObject data = p.value();

        macToCode[mac] = data["code"] | "default.ezra";


        // Build nameToMac Map (Name is Key, MAC is Value)
        String name = data["name"] | "NoName";
        macToName[mac] = name;
        nameToMac[name] = mac;
    }
    Logger::log("Loaded " + String(macToCode.size()) + " devices from SD.");
}

void Store::ensureDeleted(const String &name) {
    if (SD.exists(name)) {
        SD.remove(name);
    }
}

void Store::resetRegistry() {
    ensureDeleted("config.json");
    macToCode.clear();
    macToIp.clear();
    macToName.clear();
    nameToMac.clear();
    loadRegistryFromSD();
}

void Store::saveRegistryToSD(File &file) {
    if (!needsSave) return;
    JsonDocument doc;

    for (auto const &[mac, code]: macToCode) {
        auto formattedMac = WifiModule::macToString(mac);

        doc[formattedMac]["code"] = code;

        // Check if this MAC also has a nickname in nameToMac
        // We iterate through nameToMac to find the matching MAC
        for (auto const &[m, name]: macToName) {
            if (m == mac) {
                doc[formattedMac]["name"] = name;
                break;
            }
        }
    }
    serializeJson(doc, file);

    needsSave = false;
}

String Store::getScriptFilePathByMac(const std::array<uint8_t, 6> &mac) {
    auto assignedScriptToMac = macToCode.find(mac);
    auto macHasAssignedCodebase = assignedScriptToMac != macToCode.end();
    return String("/scripts/") + (macHasAssignedCodebase ? assignedScriptToMac->second : "waterslide.ezra");
}

String Store::readFileToString(const String &name) {
    if (File file = SD.open("/" + name, FILE_READ)) {
        const auto fileContents = file.readString();
        file.close();
        return fileContents;
    }
    return "No Content Found!";
}


void Store::initValuesFromSD() {
    initSD();
    ensureFileExists("scripts");
    ensureFileExists("hoists");

    sdQueue = xQueueCreate(60, sizeof(FileWritePacket));

    xTaskCreatePinnedToCore(sdWorkerTask, "sdWorker", 4096, nullptr, 1, nullptr, 0);
}

void Store::cleanupTempFiles() {
    File root = SD.open("/scripts");
    File f = root.openNextFile();
    while (f) {
        auto name = String(f.name());
        f.close();
        if (name.startsWith("_tmp_")) {
            SD.remove("/scripts/" + name);
            Logger::log("Cleaned up: " + name);
        }
        f = root.openNextFile();
    }
    root.close();
}
