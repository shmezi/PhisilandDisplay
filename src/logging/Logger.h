//
// Created by Ezra Golombek on 04/06/2026.
//

#ifndef PHISILANDDISPLAY_LOGGER_H
#define PHISILANDDISPLAY_LOGGER_H
#include <WString.h>


class Logger {
public:
    static void innitLogger();

    static void log(const String &contents);

    static void warn(const String &contents);

    static void error(const String &contents);

    static void remoteLog(const String &contents);
};


#endif //PHISILANDDISPLAY_LOGGER_H
