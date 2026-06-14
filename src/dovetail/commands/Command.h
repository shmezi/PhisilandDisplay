//
// Created by Ezra Golombek on 11/06/2026.
//

#ifndef PHISILANDDISPLAY_COMMAND_H
#define PHISILANDDISPLAY_COMMAND_H
#include <Arduino.h>
#include <ArduinoJson.h>

class Command {
public:
    virtual ~Command() = default;

    virtual String name() = 0;

    virtual void execute(const uint8_t &wsClientId, JsonDocument doc) = 0;
};
#endif //PHISILANDDISPLAY_COMMAND_H
