//
// Created by Ezra Golombek on 12/06/2026.
//

#ifndef PHISILANDDISPLAY_REMOTELOGCOMMAND_H
#define PHISILANDDISPLAY_REMOTELOGCOMMAND_H
#include "Command.h"


class RemoteLogCommand : public Command{
public:
    String name() override;

    void execute(const uint8_t &wsClientId, JsonDocument doc) override;
};


#endif //PHISILANDDISPLAY_REMOTELOGCOMMAND_H
