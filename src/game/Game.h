//
// Created by Ezra Golombek on 11/03/2026.
//

#ifndef PHISILANDDISPLAY_GAME_H
#define PHISILANDDISPLAY_GAME_H
#include <iosfwd>
#include <WString.h>

#include "misc/lv_event_private.h"


class Game {
public:
    static String screen;

    static void onResetButton(lv_event_t *e);

    static void onStartButton(lv_event_t *e);

    static void setCurrentScreen();

    static void onBackButton(lv_event_t *e);

    static void endRound();

    static void setA(int value);

    static void setB(int value);

    static void setC(int value);

    Game();
};


#endif //PHISILANDDISPLAY_GAME_H
