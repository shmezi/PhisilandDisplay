//
// Created by Ezra Golombek on 04/06/2026.
//

#include "Logger.h"

#include <HardwareSerial.h>

#include "Color.h"

void Logger::innitLogger() {
    while (!Serial) {
    }
}

void Logger::log(const String &contents) {
    Serial.println(apply(Color::BOLD) + apply(Color::BRIGHT_CYAN) + "[LOG] " + apply(Color::RESET) + apply(Color::BRIGHT_WHITE) + contents);
}

void Logger::warn(const String &contents) {
    Serial.println(apply(Color::BOLD) + apply(Color::YELLOW) + "[WARN] " + apply(Color::RESET) + apply(Color::BRIGHT_WHITE)+ contents);
}


void Logger::error(const String &contents) {
    Serial.println(apply(Color::BOLD) + apply(Color::RED) +"[ERROR] "  + apply(Color::RESET) + apply(Color::BRIGHT_WHITE)+ contents);
}
