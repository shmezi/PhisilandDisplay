//
// Created by Ezra Golombek on 04/06/2026.
//

#include "Logger.h"

#include <HardwareSerial.h>

void Logger::innitLogger() {
    while (!Serial) {
    }
}

void Logger::log(const String &contents) {
    Serial.println("[LOG] " + contents);
}

void Logger::warn(const String &contents) {
    Serial.println("[WARN] " + contents);
}


void Logger::error(const String &contents) {
    Serial.println("[ERROR] " + contents);
}
