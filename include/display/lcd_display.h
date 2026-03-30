#pragma once

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "models/sensor_data.h"

class LcdDisplay {
public:
    enum class DosingMode {
        NORMAL,
        PH_DOWN_CAL,
        PH_UP_CAL,
        NUTRI_A,
        NUTRI_B
    };

    LcdDisplay(uint8_t address, uint8_t columns, uint8_t rows);

    void begin();
    void setMode(DosingMode mode);
    void show(const SensorData &data, const String &rightTop, const String &rightMid, const String &rightBottom);

private:
    LiquidCrystal_I2C _lcd;
    DosingMode _mode;

    unsigned long _lastScrollMs;
    uint16_t _scrollTop;
    uint16_t _scrollMid;
    uint16_t _scrollBottom;

    static String modeToText(DosingMode mode);
    static String fitOrScroll(const String &text, uint8_t width, uint16_t &offset);
    static void printPadded(LiquidCrystal_I2C &lcd, uint8_t col, uint8_t row, const String &text, uint8_t width);
};
