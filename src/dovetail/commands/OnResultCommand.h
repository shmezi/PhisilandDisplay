//
// Created by Ezra Golombek on 12/06/2026.
//

#ifndef PHISILANDDISPLAY_ONRESULTCOMMAND_H
#define PHISILANDDISPLAY_ONRESULTCOMMAND_H
#include "Command.h"


class OnResultCommand : public Command {
public:
    String name() override;

    void execute(const uint8_t &wsClientId, JsonDocument doc) override;
};


#endif //PHISILANDDISPLAY_ONRESULTCOMMAND_H
