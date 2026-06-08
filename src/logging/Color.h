//
// Created by Ezra Golombek on 08/06/2026.
//

#ifndef PHISILANDDISPLAY_COLOR_H
#define PHISILANDDISPLAY_COLOR_H
#include <iostream>
#include <string>
#include <string_view>

enum class Color {
    BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE,
    BRIGHT_BLACK, BRIGHT_RED, BRIGHT_GREEN, BRIGHT_YELLOW, BRIGHT_BLUE, BRIGHT_MAGENTA, BRIGHT_CYAN, BRIGHT_WHITE,
    RESET, BOLD, UNDERLINE
};

// Helper function to map enum to ANSI code string literals
inline String apply(const Color color) {
    switch (color) {
        case Color::BLACK: return "\033[30m";
        case Color::RED: return "\033[31m";
        case Color::GREEN: return "\033[32m";
        case Color::YELLOW: return "\033[33m";
        case Color::BLUE: return "\033[34m";
        case Color::MAGENTA: return "\033[35m";
        case Color::CYAN: return "\033[36m";
        case Color::WHITE: return "\033[37m";

        case Color::BRIGHT_BLACK: return "\033[90m";
        case Color::BRIGHT_RED: return "\033[91m";
        case Color::BRIGHT_GREEN: return "\033[92m";
        case Color::BRIGHT_YELLOW: return "\033[93m";
        case Color::BRIGHT_BLUE: return "\033[94m";
        case Color::BRIGHT_MAGENTA: return "\033[95m";
        case Color::BRIGHT_CYAN: return "\033[96m";
        case Color::BRIGHT_WHITE: return "\033[97m";

        case Color::RESET: return "\033[0m";
        case Color::BOLD: return "\033[1m";
        case Color::UNDERLINE: return "\033[4m";
        default: return "";
    }
}


#endif //PHISILANDDISPLAY_COLOR_H
