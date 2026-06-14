//
// Created by Ezra Golombek on 10/06/2026.
//

#ifndef PHISILANDDISPLAY_FILESERVER_H
#define PHISILANDDISPLAY_FILESERVER_H

#include <functional>
#include <memory>
#include <Arduino.h>
#include <ESPAsyncWebServer.h>

struct SDResult {
    String content;
    SemaphoreHandle_t done = xSemaphoreCreateBinary();
    bool error = false;

    void sendError(const String &data) {
        error = true;
        content = data;
    }

    void sendSuccess(const String &&data) {
        content = std::move(data);
    }

    ~SDResult() {
        vSemaphoreDelete(done);
    }
};

struct SDRequest {
    std::function<void(std::shared_ptr<SDResult>)> work;
    std::shared_ptr<SDResult> result;
};

class FileServer {
public:
    static void init();

    static void dispatch(AsyncWebServerRequest *request,
                         std::function<void(std::shared_ptr<SDResult>)> work,
                         const String &contentType = "text/plain");

private:
    static QueueHandle_t _queue;

    static void _workerTask(void *pvParameters);
};


#endif //PHISILANDDISPLAY_FILESERVER_H
