//
// Created by Ezra Golombek on 11/03/2026.
//

#include "Store.h"
#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <set>
std::set<String> Store::allowedMacs;
String Store::codebase[4];

void Store::startSDCard() {
    SD.end();

    SD.begin(5, SPI, 4000000, "/sd", 10);

}
String readCodeFileToString(const String &path) {

    File file = SD.open("/" + path + ".ezra");
    if (!file) {
        Serial.println("Failed to open " + path + " script.");
        return "";
    }

    String fileContent = "";

    while (file.available()) {
        char charRead = file.read();
        fileContent += charRead;
    }

    // Close the file
    file.close();

    return fileContent;
}


void Store::initValuesFromSD() {
    while (!Serial) {
        // Wait for Serial Monitor to open
    }

    Serial.println("Initializing SD card...");

    startSDCard();

    Serial.println("Card initialized.");

    File dataFile = SD.open("/allowed-mac.txt");

    if (dataFile) {
        Serial.println("Registered mac addresses:");
        while (dataFile.available()) {
            String line = dataFile.readStringUntil('\n');
            line.trim();

            allowedMacs.insert(line);
            Serial.println("MAC: '" + line + "'"); // Print the line to the Serial Monitor
        }
        dataFile.close();
    } else {
        Serial.println("Error opening allowed-mac.txt!");
    }


    // codebase[0] = readCodeFileToString("waterslide");
    // codebase[1] = readCodeFileToString("blackmamba");
    // codebase[2] = readCodeFileToString("swings");
    // codebase[3] = readCodeFileToString("ferriswheel");
}

void Store::saveToMacList() {

    startSDCard();

    // Opening with FILE_WRITE replaces the old file
    File file = SD.open("/allowed-mac.txt", FILE_WRITE);

    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }

    // Iterate through the set and write each element
    for (const auto &value: allowedMacs) {
        file.println(value);
    }

    file.close();
    Serial.println("Set successfully saved (replaced old file).");

}
