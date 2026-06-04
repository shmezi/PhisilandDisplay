//
// Created by Ezra Golombek on 11/03/2026.
//

#ifndef PHISILANDDISPLAY_STORE_H
#define PHISILANDDISPLAY_STORE_H
#include <set>
#include <Arduino.h>
#include <FS.h>
#include <map>
#include <bits/stl_vector.h>


class Store {
public:
    static std::set<String> allowedMacs;
    static String codebase[4];

    static std::vector<String> registeredMacsToVerify;

    static std::map<String, IPAddress> macToIp;

    static std::map<String, String> macToName;

    static std::map<String, String> nameToMac;

    static std::map<String, String> macToCode;

    static bool needsSave;

    static void initSD();

    static bool ensureFileExists(const String &name);

    static bool getFileOrCreateDefault(const String &name, const std::function<bool(File &file)>& defaultValue);

    static void loadRegistryFromSD();

    static void saveRegistryToSD(File &file);

    static void initValuesFromSD();

};


#endif //PHISILANDDISPLAY_STORE_H
