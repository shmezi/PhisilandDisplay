//
// Created by Ezra Golombek on 11/03/2026.
//

#ifndef PHISILANDDISPLAY_STORE_H
#define PHISILANDDISPLAY_STORE_H
#include <set>
#include <Arduino.h>
#include <SD.h>
#include "devices/ClientId.h"
#include <map>
#include <bits/stl_vector.h>
#include <ArduinoJson.h>

#include "hoist/HoistSystem.h"


class Store {
public:
    static SemaphoreHandle_t needsSave;


    static void initSD();


    static bool ensureFileExists(const String &name);

    static bool getFileOrCreateDefault(const String &name, const std::function<bool(File &file)> &defaultValue);

    static bool isStandardFile(File &file);

    static String hoistEntriesForHoistSelection;

    static void registerHoistId(const String &id);

    static ClientConfig loadClientFromVariant(const JsonVariant &clientDocument);

    static Hoist loadHoistFromDocument(JsonDocument &hoistDocument);

    static void loadHoists();

    static void loadRegistryFromSD();

    static void ensureDeleted(const String &name);

    static void resetRegistry();



    static String readFileToString(const String &name);

    static void initValuesFromSD();

    static void cleanupTempFiles();
};


#endif //PHISILANDDISPLAY_STORE_H
