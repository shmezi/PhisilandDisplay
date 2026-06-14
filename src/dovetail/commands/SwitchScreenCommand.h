//
// Created by Ezra Golombek on 12/06/2026.
//

#ifndef PHISILANDDISPLAY_SWITCHSCREENCOMMAND_H
#define PHISILANDDISPLAY_SWITCHSCREENCOMMAND_H
#include "Command.h"


class SwitchScreenCommand : public Command{
public:
    String name() override;

    void execute(const uint8_t &wsClientId, JsonDocument doc) override;
};


#endif //PHISILANDDISPLAY_SWITCHSCREENCOMMAND_H
