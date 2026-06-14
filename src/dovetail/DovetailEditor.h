//
// Created by Ezra Golombek on 04/06/2026.
//

#ifndef PHISILANDDISPLAY_DOVETAILEDITOR_H
#define PHISILANDDISPLAY_DOVETAILEDITOR_H
#include "ESPAsyncWebServer.h"


class DovetailEditor {
    static std::vector<uint8_t> htmlBuffer;

public:
    static void initEditorRoutes();

    static void readFile(AsyncWebServerRequest *request);

    static void listDevices(AsyncWebServerRequest *request);

    static void renameDevice(AsyncWebServerRequest *request);

    static void deleteScript(AsyncWebServerRequest *request);

    static void renameScript(AsyncWebServerRequest *request);

    static void saveFile(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);


    static void listScripts(AsyncWebServerRequest *request);

    static void runScript(AsyncWebServerRequest *request);

    static bool cacheWebpageToRAM();

    static void webpage(AsyncWebServerRequest *request);
};


#endif //PHISILANDDISPLAY_DOVETAILEDITOR_H
