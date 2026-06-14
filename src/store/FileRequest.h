//
// Created by Ezra Golombek on 10/06/2026.
//

#ifndef PHISILANDDISPLAY_FILEREQUEST_H
#define PHISILANDDISPLAY_FILEREQUEST_H
#include <memory>
#include <Arduino.h>

struct FileReadResult {
    String content;
    bool ready = false;
    bool error = false;
};

struct FileReadRequest {
    char path[64];
    std::shared_ptr<FileReadResult> result;
};
#endif //PHISILANDDISPLAY_FILEREQUEST_H
