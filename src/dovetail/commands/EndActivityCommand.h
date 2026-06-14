//
// Created by Ezra Golombek on 12/06/2026.
//

#ifndef PHISILANDDISPLAY_ENDACTIVITYCOMMAND_H
#define PHISILANDDISPLAY_ENDACTIVITYCOMMAND_H
#include "Command.h"


class EndActivityCommand : public Command{
public:
    String name() override;

    void execute(const uint8_t &wsClientId, JsonDocument doc) override;
};


#endif //PHISILANDDISPLAY_ENDACTIVITYCOMMAND_H
