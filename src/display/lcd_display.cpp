#include "display/lcd_display.h"

LcdDisplay::LcdDisplay(uint8_t address, uint8_t columns, uint8_t rows)
    : _lcd(address, columns, rows) {}

void LcdDisplay::begin() {
    _lcd.init();
    _lcd.backlight();
    _lcd.clear();
}

String LcdDisplay::buildDateString() const {
    return String("DATE:N/A");
}

void LcdDisplay::printPadded(LiquidCrystal_I2C &lcd, uint8_t col, uint8_t row, const String &text, uint8_t width) {
    String out = text;
    if (out.length() > width) {
        out = out.substring(0, width);
    }

    lcd.setCursor(col, row);
    lcd.print(out);

    for (uint8_t i = out.length(); i < width; i++) {
        lcd.print(' ');
    }
}

void LcdDisplay::show(const SensorData &data, const char *modeText) {
    String tempText = "T:" + String(data.temperatureC, 1) + "C";
    String farm = "Hasna's Farm";

    printPadded(_lcd, 0, 0, tempText, 9);
    printPadded(_lcd, 9, 0, farm, 11);

    String tdsText = "TDS: " + String(data.tds, 0) + " ppm";
    printPadded(_lcd, 0, 1, tdsText, 20);

    String phText = "pH: " + String(data.phValue, 2);
    String dateText = buildDateString();
    if (dateText.length() > 10) {
        dateText = dateText.substring(0, 10);
    }

    printPadded(_lcd, 0, 2, phText, 9);
    printPadded(_lcd, 10, 2, dateText, 10);

    String mode = String("Mode: ") + modeText;
    printPadded(_lcd, 0, 3, mode, 20);
}
