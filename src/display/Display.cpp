//
// Created by Ezra Golombek on 11/03/2026.
//

#include "Display.h"

#include "lvgl.h"
#include "lv_api_map_v8.h"
#include "TFT_eSPI.h"
#include "display/lv_display.h"
#include "indev/lv_indev.h"
#include "misc/lv_types.h"
#include "ui_output/ui.h"
#include <XPT2046_Bitbang.h>


lv_indev_t *Display::indev;
uint8_t *Display::draw_buf;

uint32_t Display::lastTick = 0;

#define MOSI_PIN 32
#define MISO_PIN 39
#define CLK_PIN  25
#define CS_PIN   33

XPT2046_Bitbang Display::touchscreen(MOSI_PIN, MISO_PIN, CLK_PIN, CS_PIN);
uint16_t Display::touchScreenMinimumX = 1000;
uint16_t Display::touchScreenMaximumX = 0;
uint16_t Display::touchScreenMinimumY = 1000;
uint16_t Display::touchScreenMaximumY = 0;


auto tft = TFT_eSPI(); // Manually define the object

bool Display::detectInversionRequirement() {
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    Serial.println("Data: ");
    Serial.println(chip_info.model);
    Serial.println(chip_info.revision);
    Serial.println(chip_info.full_revision);

    return chip_info.revision == 1;
}

/* LVGL calls it when a rendered image needs to copied to the display*/
void Display::my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    /*Call it to tell LVGL you are ready*/
    lv_disp_flush_ready(disp);
}

void Display::my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
    const auto p = touchscreen.getTouch();

    // Use a small threshold. p.zRaw != 0 is often too twitchy
    if (p.zRaw > 30) {
        // Update Calibration Limits (Ensure these variables are initialized!)
        if (p.x < touchScreenMinimumX) touchScreenMinimumX = p.x;
        if (p.x > touchScreenMaximumX) touchScreenMaximumX = p.x;
        if (p.y < touchScreenMinimumY) touchScreenMinimumY = p.y;
        if (p.y > touchScreenMaximumY) touchScreenMaximumY = p.y;

        // Prevent divide-by-zero on the very first touch
        if (touchScreenMaximumX <= touchScreenMinimumX) {
            data->state = LV_INDEV_STATE_RELEASED;
            return;
        }
        // 1. Map the RAW Y to the SCREEN X (Horizontal)
        // We use 0 to RES-1 to "un-flip" the inversion you're seeing
        int32_t x_mapped = map(p.y, touchScreenMinimumY, touchScreenMaximumY, 0, TFT_HORI_RES - 1);

        // 2. Map the RAW X to the SCREEN Y (Vertical)
        int32_t y_mapped = map(p.x, touchScreenMinimumX, touchScreenMaximumX,  TFT_VERI_RES - 1,0);

        // 3. Constrain and Assign
        data->point.x = (lv_coord_t)constrain(x_mapped, 0, TFT_HORI_RES - 1);
        data->point.y = (lv_coord_t)constrain(y_mapped, 0, TFT_VERI_RES - 1);
        data->state = LV_INDEV_STATE_PRESSED;
        // Serial.printf("Raw(%d,%d) -> Pixel(%d,%d)\n", p.x, p.y, data->point.x, data->point.y);
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void Display::lvglTask() {
    lv_tick_inc(millis() - lastTick); //Update the tick timer. Tick is new for LVGL 9
    lastTick = millis();
    lv_timer_handler();
}

void Display::innit() {
    String LVGL_Arduino = "LVGL demo ";

    LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();


    Serial.println(LVGL_Arduino);
    Serial.setTimeout(20); // short timeout for safety

    pinMode(21, OUTPUT);
    digitalWrite(21, HIGH);
    //Initialise the touchscreen

    /* Start second SPI bus for touchscreen */
    touchscreen.begin(); /* Touchscreen init */
    // touchscreen.setCalibration(); /* Inverted landscape orientation to match screen */

    //Initialise LVGL
    lv_init();
    draw_buf = new uint8_t[DRAW_BUF_SIZE];
    lv_display_t *disp;
    tft.begin();
    tft.setRotation(2); // Set to 1 for Landscape
    tft.setSwapBytes(true); // FIXES THE "TRASH" COLORS/STATIC
    tft.fillScreen(TFT_BLACK); // Clear the garbled memory

    disp = lv_tft_espi_create(TFT_HORI_RES, TFT_VERI_RES, draw_buf, DRAW_BUF_SIZE);
    // touchscreen.setRotation(2);
    lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90); // <<-- Add line

    delay(1000);
    //
    if (detectInversionRequirement()) {
        Serial.println("Chip XXRS69 detected. Inverting colors...");
        tft.invertDisplay(true);
    } else {
        Serial.println("Chip XH-32S detected. Standard colors applied.");
        tft.invertDisplay(false);
    }


    //Initialize the XPT2046 input device driver
    indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touchpad_read);

    ui_init();
}
