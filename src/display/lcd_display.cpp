#include "display/lcd_display.h"
#include "config/app_config.h"

LcdDisplay::LcdDisplay(uint8_t address, uint8_t columns, uint8_t rows)
    : _lcd(address, columns, rows),
      _mode(DosingMode::NORMAL),
      _lastScrollMs(0),
      _scrollTop(0),
      _scrollMid(0),
      _scrollBottom(0) {}

void LcdDisplay::begin() {
    _lcd.init();
    _lcd.backlight();
    _lcd.clear();
}

void LcdDisplay::setMode(DosingMode mode) {
    _mode = mode;
}

String LcdDisplay::modeToText(DosingMode mode) {
    switch (mode) {
        case DosingMode::PH_DOWN_CAL:
            return "PH v CAL";
        case DosingMode::PH_UP_CAL:
            return "PH ^ CAL";
        case DosingMode::NUTRI_A:
            return "NUTRI A";
        case DosingMode::NUTRI_B:
            return "NUTRI B";
        case DosingMode::NORMAL:
        default:
            return "NORMAL";
    }
}

String LcdDisplay::fitOrScroll(const String &text, uint8_t width, uint16_t &offset) {
    if (text.length() <= width) {
        offset = 0;
        return text;
    }

    String padded = text + "   ";
    uint16_t maxOffset = padded.length();
    if (offset >= maxOffset) {
        offset = 0;
    }

    String view;
    view.reserve(width);
    for (uint8_t i = 0; i < width; i++) {
        uint16_t idx = (offset + i) % maxOffset;
        view += padded[idx];
    }
    return view;
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

void LcdDisplay::show(const SensorData &data, const String &rightTop, const String &rightMid, const String &rightBottom) {
    constexpr uint8_t LEFT_W = 11;
    constexpr uint8_t RIGHT_W = 9;

    unsigned long now = millis();
    if (now - _lastScrollMs >= AppConfig::LCD_SCROLL_INTERVAL_MS) {
        _lastScrollMs = now;
        _scrollTop++;
        _scrollMid++;
        _scrollBottom++;
    }

    // Bagian kiri (fix)
    printPadded(_lcd, 0, 0, "Temp:" + String(data.temperatureC, 1) + "C", LEFT_W);
    printPadded(_lcd, 0, 1, "TDS:" + String(data.tds, 0) + "ppm", LEFT_W);
    printPadded(_lcd, 0, 2, "pH:" + String(data.phValue, 2), LEFT_W);
    printPadded(_lcd, 0, 3, "Mode:" + modeToText(_mode), LEFT_W);

    // Bagian kanan (running text jika tidak muat)
    printPadded(_lcd, LEFT_W, 0, fitOrScroll(rightTop, RIGHT_W, _scrollTop), RIGHT_W);
    printPadded(_lcd, LEFT_W, 1, fitOrScroll(rightMid, RIGHT_W, _scrollMid), RIGHT_W);
    printPadded(_lcd, LEFT_W, 2, fitOrScroll(rightBottom, RIGHT_W, _scrollBottom), RIGHT_W);
    printPadded(_lcd, LEFT_W, 3, "", RIGHT_W);
}
