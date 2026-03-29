#pragma once

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "models/sensor_data.h"

class LcdDisplay {
public:
    LcdDisplay(uint8_t address, uint8_t columns, uint8_t rows);

    void begin();
    void show(const SensorData &data, const char *modeText);

private:
    LiquidCrystal_I2C _lcd;

    String buildDateString() const;
    static void printPadded(LiquidCrystal_I2C &lcd, uint8_t col, uint8_t row, const String &text, uint8_t width);
};
