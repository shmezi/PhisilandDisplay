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

    static bool shouldSwitchScreen;

    static bool shouldUpdateValues;


    static String aValue;

    static String cValue;

    static String bValue;

    static void updateValues();

    static void setCurrentScreen();

    static void onBackButton(lv_event_t *e);

    static void endRound();

    static void setA(const String &value);

    static void setB(const String &value);

    static void setC(const String &value);

};


#endif //PHISILANDDISPLAY_GAME_H
