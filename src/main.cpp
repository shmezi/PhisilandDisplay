/* Using LVGL with Arduino requires some extra steps...
 *
 * Be sure to read the docs here: https://docs.lvgl.io/master/integration/framework/arduino.html
 * but note you should use the lv_conf.h from the repo as it is pre-edited to work.
 *
 * You can always edit your own lv_conf.h later and exclude the example options once the build environment is working.
 *
 * Note you MUST move the 'examples' and 'demos' folders into the 'src' folder inside the lvgl library folder
 * otherwise this will not compile, please see README.md in the repo.
 *
 */
#include <lvgl.h>

#include <TFT_eSPI.h>

#include <XPT2046_Touchscreen.h>

#include "ui_output/ui.h"

// A library for interfacing with the touch screen
//
// Can be installed from the library manager (Search for "XPT2046")
//https://github.com/PaulStoffregen/XPT2046_Touchscreen
// ----------------------------
// Touch Screen pins
// ----------------------------

// The CYD touch uses some non default
// SPI pins

#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

int screen = 1;

SPIClass touchscreenSpi = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);
uint16_t touchScreenMinimumX = 200, touchScreenMaximumX = 3700, touchScreenMinimumY = 240, touchScreenMaximumY = 3800;

/*Set to your screen resolution*/
#define TFT_HORI_RES  240
#define TFT_VERI_RES   320

/*LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes*/
#define DRAW_BUF_SIZE (TFT_HORI_RES * TFT_VERI_RES / 10 * (LV_COLOR_DEPTH / 8))

#if LV_USE_LOG != 0
void my_print(lv_log_level_t level, const char *buf) {
    LV_UNUSED(level);
    Serial.println(buf);
    Serial.flush();
}
#endif

/* LVGL calls it when a rendered image needs to copied to the display*/
void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    /*Call it to tell LVGL you are ready*/
    lv_disp_flush_ready(disp);
}

/*Read the touchpad*/
void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
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
        /*
        Serial.print("Touch x ");
        Serial.print(data->point.x);
        Serial.print(" y ");
        Serial.println(data->point.y);
        */
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

/*

* Phisics System from this point onward.

*/
void onStartButton(lv_event_t *e) {
    if (!(lv_obj_has_state(ui_StartStop, LV_STATE_CHECKED) || lv_obj_has_state(ui_StartStop1, LV_STATE_CHECKED) ||
          lv_obj_has_state(ui_StartStop2, LV_STATE_CHECKED))) {
        Serial.print("~-1");
        return;
    }

    switch (screen) {
        case 1:
            Serial.print("~-2");
            break;

        case 2:
            Serial.print(String("~") + String(lv_arc_get_value(ui_SpeedControl1)));
            break;
        case 3:
            Serial.print(String("~") + String(lv_arc_get_value(ui_SpeedControl2)));
            break;
    }
}

void setCurrentScreen() {
    if (screen == 1) {
        lv_disp_load_scr(ui_WaterParkStart);
    }
    if (screen == 2) {
        lv_disp_load_scr(ui_SwingStart);
    }
    if (screen == 3) {
        lv_disp_load_scr(ui_BlackMambaStart);
    }
}

void onBackButton(lv_event_t *e) {
    setCurrentScreen();
}

lv_indev_t *indev; //Touchscreen input device
uint8_t *draw_buf; //draw_buf is allocated on heap otherwise the static area is too big on ESP32 at compile
uint32_t lastTick = 0; //Used to track the tick timer

TFT_eSPI tft = TFT_eSPI(); // Manually define the object

bool detectInversionRequirement() {
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    Serial.println("Data: ");
    Serial.println(chip_info.model);
    Serial.println(chip_info.revision);
    Serial.println(chip_info.full_revision);

    return chip_info.revision == 1;
}


void setup() {
    //Some basic info on the Serial console
    String LVGL_Arduino = "LVGL demo ";
    LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
    Serial.begin(115200);
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
    // if (detectInversionRequirement()) {
    //     Serial.println("Chip XXRS69 detected. Inverting colors...");
    //     tft.invertDisplay(true);
    // } else {
    //     Serial.println("Chip XH-32S detected. Standard colors applied.");
    //     tft.invertDisplay(false);
    // }


    //Initialize the XPT2046 input device driver
    indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touchpad_read);

    ui_init();
    // // //Done
    //       xTaskCreatePinnedToCore(
    //           lvglTask,       // Task function
    //           "LVGL Task",    // Name of task
    //           4096,           // Stack size (bytes)
    //           NULL,           // Parameter to pass
    //           1,              // Task priority (higher is more urgent)
    //           NULL,           // Task handle
    //           1               // Core to run on (Core 1 for LVGL)
    //       );
    Serial.println("Setup done");
}

void endRound() {
    lv_obj_clear_state(ui_StartStop, LV_STATE_CHECKED);

    lv_obj_clear_state(ui_StartStop1, LV_STATE_CHECKED);
    lv_obj_clear_state(ui_StartStop2, LV_STATE_CHECKED);
    lv_label_set_text(ui_StartStopLabel, "Start");
    lv_label_set_text(ui_StartStopLabel1, "Start");
    lv_label_set_text(ui_StartStopLabel2, "Start");

    lv_obj_clear_state(ui_SpeedControl1, LV_STATE_DISABLED);
    lv_obj_clear_state(ui_SpeedControl2, LV_STATE_DISABLED);
}

void setA(int value) {
    auto finalValue = String(value);
    lv_label_set_text(ui_SensorAValue, (finalValue + "ms").c_str());
    lv_label_set_text(ui_SensorAValue1, (finalValue + "°").c_str());
    lv_label_set_text(ui_SensorAValue2, (finalValue + "ms").c_str());
}


void setB(int value) {
    auto finalValue = String(value);
    lv_label_set_text(ui_SensorBValue, (finalValue + "ms").c_str());
    lv_label_set_text(ui_SensorBValue1, (finalValue + "°").c_str());
}

void setC(int value) {
    auto finalValue = String(value);
    lv_label_set_text(ui_SensorCValue, (finalValue + "ms").c_str());

    lv_label_set_text(ui_DataResult, (finalValue + " Rotations").c_str());
}

void lvglTask() {
    lv_tick_inc(millis() - lastTick); //Update the tick timer. Tick is new for LVGL 9
    lastTick = millis();
    lv_timer_handler();
}


char receivedChars[64]; // Buffer to store the received data

void loop() {
    lvglTask();
    byte numBytesRead = Serial.readBytesUntil('\n', receivedChars, sizeof(receivedChars) - 1);

    if (numBytesRead > 0) {
        receivedChars[numBytesRead] = '\0'; // Null-terminate the string at the end of the progra,


        String ardString = String(receivedChars);
        switch (receivedChars[0]) {
            case 'a':
                setA(atoi(receivedChars + 2));
                break;
            case 'b':
                setB(atoi(receivedChars + 2));
                break;
            case 'c':
                setC(atoi(receivedChars + 2));
                break;

            case '1':
                screen = 1;
                setCurrentScreen();
                break;
            case '2':
                screen = 2;
                setCurrentScreen();
                break;
            case '3':
                screen = 3;
                setCurrentScreen();
                break;
            case 's':
                endRound();
                break;
        }
    }

    delay(5);
}
