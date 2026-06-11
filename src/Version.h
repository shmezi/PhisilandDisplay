//
// Created by Ezra Golombek on 11/06/2026.
//

#ifndef PHISILANDDISPLAY_VERSION_H
#define PHISILANDDISPLAY_VERSION_H
#include <Arduino.h>
inline constexpr int MAJOR = 1, MINOR = 0, PATCH = 0;
inline const String VERSION = String(MAJOR) + "." + String(MINOR) + "." + String(PATCH);

#endif //PHISILANDDISPLAY_VERSION_H
