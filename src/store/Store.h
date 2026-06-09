//
// Created by Ezra Golombek on 11/03/2026.
//

#ifndef PHISILANDDISPLAY_STORE_H
#define PHISILANDDISPLAY_STORE_H
#include <set>
#include <Arduino.h>
#include <SD.h>

#include <map>
#include <bits/stl_vector.h>
#include <ArduinoJson.h>

#include "hoist/HoistSystem.h"


class Store {
public:
    static std::vector<std::array<uint8_t, 6> > registeredMacsToVerify;

    static std::map<std::array<uint8_t, 6>, IPAddress> macToIp;

    static std::map<std::array<uint8_t, 6>, String> macToName;

    static std::map<String, std::array<uint8_t, 6> > nameToMac;

    static std::map<std::array<uint8_t, 6>, String> macToCode;

    static bool needsSave;

    static void initSD();

    static QueueHandle_t sdQueue;

    static bool ensureFileExists(const String &name);

    static bool getFileOrCreateDefault(const String &name, const std::function<bool(File &file)> &defaultValue);

    static bool isStandardFile(File &file);

    static String hoistEntriesForHoistSelection;

    static void registerHoistId(const String &id);

    static ClientConfig loadClientFromVariant(const ArduinoJson::JsonVariant &clientDocument);

    static Hoist loadHoistFromDocument(ArduinoJson::JsonDocument &hoistDocument);

    static void loadHoists();

    static void loadRegistryFromSD();

    static void ensureDeleted(const String &name);

    static void resetRegistry();

    static void saveRegistryToSD(File &file);

    static String getScriptFilePathByMac(const std::array<uint8_t, 6> &mac);

    static String readFileToString(const String &name);

    static void initValuesFromSD();

    static void cleanupTempFiles();
};


#endif //PHISILANDDISPLAY_STORE_H
