//
// Created by Ezra Golombek on 11/03/2026.
//

#ifndef PHISILANDDISPLAY_STORE_H
#define PHISILANDDISPLAY_STORE_H
#include <set>
#include <Arduino.h>


class Store {
public:
    static std::set<String> allowedMacs;
    static String codebase[4];

    static void startSDCard();

    static void initValuesFromSD();

    static void saveToMacList();
};


#endif //PHISILANDDISPLAY_STORE_H
