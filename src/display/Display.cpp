//
// Created by Ezra Golombek on 11/03/2026.
//

#include "Display.h"

#include "lvgl.h"
#include "lv_api_map_v8.h"
#include "TFT_eSPI.h"
#include "XPT2046_Touchscreen.h"
#include "display/lv_display.h"
#include "indev/lv_indev.h"
#include "misc/lv_types.h"
#include "ui_output/ui.h"


lv_indev_t *Display::indev;
uint8_t *Display::draw_buf;

uint32_t Display::lastTick = 0;
SPIClass Display::touchscreenSpi = SPIClass(VSPI);

XPT2046_Touchscreen Display::touchscreen = XPT2046_Touchscreen(XPT2046_CS, XPT2046_IRQ);
uint16_t Display::touchScreenMinimumX = 200;
uint16_t Display::touchScreenMaximumX = 3700;
uint16_t Display::touchScreenMinimumY = 240;
uint16_t Display::touchScreenMaximumY = 3800;


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

/*Read the touchpad*/
void Display::my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
    if (touchscreen.touched()) {
        TS_Point p = touchscreen.getPoint();
        //Some very basic auto calibration so it doesn't go out of range
        if (p.x < touchScreenMinimumX) touchScreenMinimumX = p.x;
        if (p.x > touchScreenMaximumX) touchScreenMaximumX = p.x;
        if (p.y < touchScreenMinimumY) touchScreenMinimumY = p.y;
        if (p.y > touchScreenMaximumY) touchScreenMaximumY = p.y;
        //Map this to the pixel position
        data->point.x = map(p.x, touchScreenMinimumX, touchScreenMaximumX, 1,TFT_HORI_RES);
        /* Touchscreen X calibration */
        data->point.y = map(p.y, touchScreenMinimumY, touchScreenMaximumY, 1,TFT_VERI_RES);
        /* Touchscreen Y calibration */
        data->state = LV_INDEV_STATE_PRESSED;
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

    touchscreenSpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    /* Start second SPI bus for touchscreen */
    touchscreen.begin(touchscreenSpi); /* Touchscreen init */
    touchscreen.setRotation(0); /* Inverted landscape orientation to match screen */

    //Initialise LVGL
    lv_init();
    draw_buf = new uint8_t[DRAW_BUF_SIZE];
    lv_display_t *disp;
    tft.begin();
    tft.setRotation(2); // Set to 1 for Landscape
    tft.setSwapBytes(true); // FIXES THE "TRASH" COLORS/STATIC
    tft.fillScreen(TFT_BLACK); // Clear the garbled memory

    disp = lv_tft_espi_create(TFT_HORI_RES, TFT_VERI_RES, draw_buf, DRAW_BUF_SIZE);
    touchscreen.setRotation(2);
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
