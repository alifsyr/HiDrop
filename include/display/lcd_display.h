#pragma once

#include <Arduino.h>
#include <time.h>
#include <LiquidCrystal_I2C.h>
#include "models/display_mode.h"
#include "models/sensor_data.h"

class LcdDisplay {
public:
    LcdDisplay(uint8_t address, uint8_t columns, uint8_t rows);

    void begin();
    void showInitializing();
    void showInitializingFinish();
    void show(
        const SensorData &data,
        DisplayMode mode,
        bool wifiConnected,
        const struct tm *localTime,
        bool timeValid
    );

private:
    LiquidCrystal_I2C _lcd;
    unsigned long _lastScrollMs;
    size_t _rightScrollOffsets[3];

    String buildDateString(const struct tm &localTime) const;
    String buildModeLabel(DisplayMode mode) const;
    String buildRightWindow(const String &text, uint8_t width, size_t offset) const;
    String buildTimeString(const struct tm &localTime) const;
    void showStatusScreen(const String &line1, const String &line2, const String &line3, const String &line4);
    void updateScrollState(const String rightTexts[], uint8_t count);
    static void printCentered(LiquidCrystal_I2C &lcd, uint8_t row, const String &text, uint8_t width);
    static void printPadded(LiquidCrystal_I2C &lcd, uint8_t col, uint8_t row, const String &text, uint8_t width);
};
