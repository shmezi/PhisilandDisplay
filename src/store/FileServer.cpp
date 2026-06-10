//
// Created by Ezra Golombek on 10/06/2026.
//

#include "FileServer.h"

#include <SD.h>
#include "SDLock.h"

QueueHandle_t FileServer::_queue;

void FileServer::init() {
    _queue = xQueueCreate(10, sizeof(SDRequest));
    xTaskCreatePinnedToCore(_workerTask, "fileServer", 4096, nullptr, 1, nullptr, 0);
}

void FileServer::dispatch(
    AsyncWebServerRequest *request,
    std::function<void(std::shared_ptr<SDResult>)> work,
    const String &contentType) {
    auto result = std::make_shared<SDResult>();
    SDRequest req{work, result};

    if (xQueueSend(_queue, &req, 0) != pdTRUE) {
        request->send(503, "text/plain", "Server busy");
        return;
    }

    request->send(request->beginChunkedResponse(
        contentType,
        [result](uint8_t *buf, size_t maxLen, size_t index) -> size_t {
            if (index == 0) {
                xSemaphoreTake(result->done, portMAX_DELAY); // ← wait once
            }
            if (result->error) return 0;
            if (index >= result->content.length()) return 0;
            size_t len = min(maxLen, result->content.length() - index);
            memcpy(buf, result->content.c_str() + index, len);
            return len;
        }
    ));
}

void FileServer::_workerTask(void *pvParameters) {
    SDRequest req;
    while (true) {
        if (xQueueReceive(_queue, &req, portMAX_DELAY)) {
            SDLock lock;
            req.work(req.result);
            xSemaphoreGive(req.result->done);
        }
    }
}
