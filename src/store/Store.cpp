//
// Created by Ezra Golombek on 11/03/2026.
//

#include "Store.h"
#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <set>

#include "FileServer.h"
#include "SDLock.h"
#include "devices/DeviceManager.h"
#include "dovetail/DovetailEditor.h"
#include "dovetail/DovetailSystem.h"
#include "dovetail/WifiModule.h"
#include "hoist/HoistSystem.h"
#include "logging/Logger.h"
#include "widgets/roller/lv_roller.h"
#include "wsFileServer/WSFileServer.h"


struct ClientConfig;

//Hey future confused me, Ezra from 9/06, I haven't checked but I think this needs save is regarding the config file.
//If it's wrong, just change it's name.. Ezra from the past really should have named stuff better aye?
SemaphoreHandle_t Store::needsSave;


void Store::initSD() {
    Logger::log("Initializing SD card...");
    SD.begin(5, SPI, 4000000, "/sd", 10);
    Logger::log("Card initialized.");
}


String readScriptFileToString(const String &path) {
    SDLock lock;
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
    SDLock lock;
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
    SDLock lock;
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

    const auto ClientId = device["id"].as<String>();
    const auto deviceDescription = device["description"].as<String>();
    const auto deviceFile = device["file"].as<String>();
    return {ClientId, deviceDescription, deviceFile};
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
    SDLock lock;
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
        const auto doc = DeviceManager::getInstance().serializeDevicesToJson();
        serializeJson(doc, f);
        return true;
    });

    SDLock lock;

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

    DeviceManager::getInstance().loadDevicesToCache(doc);
}

void Store::ensureDeleted(const String &name) {
    SDLock lock;
    if (SD.exists(name)) {
        SD.remove(name);
    }
}

void Store::resetRegistry() {
    ensureDeleted("config.json");
    DeviceManager::getInstance().clearDeviceCache();
    loadRegistryFromSD();
}


String Store::readFileToString(const String &name) {
    SDLock lock;
    if (File file = SD.open("/" + name, FILE_READ)) {
        const auto fileContents = file.readString();
        file.close();
        return fileContents;
    }
    return "No Content Found!";
}


void storeConfigLoop(void *pvParameters) {
    for (;;) {
        if (xSemaphoreTake(Store::needsSave, portMAX_DELAY) == pdTRUE) {
            const auto doc = DeviceManager::getInstance().serializeDevicesToJson();
            if (doc.overflowed()) {
                Logger::error("doc overflow!");
                return;
            }
            SDLock lock;
            auto f = SD.open("/config.json", FILE_WRITE);
            serializeJson(doc, f);
            f.close();
        }
    }
}

void Store::initValuesFromSD() {
    needsSave = xSemaphoreCreateBinary();
    initSD();
    ensureFileExists("scripts");
    ensureFileExists("hoists");
    xTaskCreatePinnedToCore(storeConfigLoop, "needingSave", 4096, nullptr, 1, nullptr, 0);

    loadRegistryFromSD();
    DovetailEditor::cacheWebpageToRAM();
    // FileServer::init();
    WSFileServer::init(DovetailSystem::server);
}

void Store::cleanupTempFiles() {
    SDLock lock;
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
