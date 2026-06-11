//
// Created by Ezra Golombek on 11/06/2026.
//

#ifndef PHISILANDDISPLAY_REGISTERCOMMAND_H
#define PHISILANDDISPLAY_REGISTERCOMMAND_H
#include "Command.h"


class RegisterCommand  : public Command{
public:
    String name() override;

    void execute(const uint8_t &wsClientID, JsonDocument doc) override;
};


#endif //PHISILANDDISPLAY_REGISTERCOMMAND_H
