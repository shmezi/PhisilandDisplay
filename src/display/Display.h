//
// Created by Ezra Golombek on 11/03/2026.
//

#ifndef PHISILANDDISPLAY_DISPLAY_H
#define PHISILANDDISPLAY_DISPLAY_H
#include <SPI.h>

#include "XPT2046_Bitbang.h"
#include "display/lv_display_private.h"

// SPI pins:
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

// Screen resolution:
#define TFT_HORI_RES  240
#define TFT_VERI_RES   320


class Display {
    /*
     * LVGL Params:
     */
    static lv_indev_t *indev;
    static uint8_t *draw_buf; //draw_buf is allocated on heap otherwise the static area is too big on ESP32 at compile
    static uint32_t lastTick; //Used to track the tick timer

    static XPT2046_Bitbang touchscreen;

    static uint16_t touchScreenMinimumX;
    static uint16_t touchScreenMaximumX;
    static uint16_t touchScreenMinimumY;
    static uint16_t touchScreenMaximumY;


    /*LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes*/
#define DRAW_BUF_SIZE (TFT_HORI_RES * TFT_VERI_RES / 10 * (LV_COLOR_DEPTH / 8))

#if LV_USE_LOG != 0
    void my_print(lv_log_level_t level, const char *buf) {
        LV_UNUSED(level);
        Serial.println(buf);
        Serial.flush();
    }
#endif


    static bool detectInversionRequirement();

    static void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);

    static void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data);

public:
    static void lvglTask();

    static void innit();
};


#endif //PHISILANDDISPLAY_DISPLAY_H
