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

    static std::vector<String> registeredMacsToVerify;

    static std::map<String, IPAddress> macToIp;

    static std::map<String, String> macToName;

    static std::map<String, String> nameToMac;

    static std::map<String, String> macToCode;

    static bool needsSave;

    static void initSD();

    static QueueHandle_t sdQueue;

    static bool ensureFileExists(const String &name);

    static bool getFileOrCreateDefault(const String &name, const std::function<bool(File &file)>& defaultValue);

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

    static String getScriptFilePathByMac(const String&);

    static String readFileToString(const String &name);

    static void initValuesFromSD();

};


#endif //PHISILANDDISPLAY_STORE_H
