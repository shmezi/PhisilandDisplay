//
// Created by Ezra Golombek on 10/06/2026.
//

#ifndef PHISILANDDISPLAY_SDLOCK_H
#define PHISILANDDISPLAY_SDLOCK_H
#include <Arduino.h>
#include <freertos/semphr.h>

extern SemaphoreHandle_t sdMutex;

class SDLock {
public:
    SDLock() { xSemaphoreTakeRecursive(sdMutex, portMAX_DELAY); }
    ~SDLock() { xSemaphoreGiveRecursive(sdMutex); }
};


#endif //PHISILANDDISPLAY_SDLOCK_H
