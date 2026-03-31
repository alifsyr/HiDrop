#include "display/lcd_display.h"

#include "config/app_config.h"

namespace {
constexpr uint8_t kArrowDownCharSlot = 1;
constexpr uint8_t kArrowUpCharSlot = 2;
constexpr uint8_t kLeftWidth = 11;
constexpr uint8_t kDividerColumn = 11;
constexpr uint8_t kRightColumn = 12;
constexpr uint8_t kRightWidth = 8;
constexpr uint8_t kRightLineCount = 3;
}

LcdDisplay::LcdDisplay(uint8_t address, uint8_t columns, uint8_t rows)
    : _lcd(address, columns, rows),
      _lastScrollMs(0),
      _rightScrollOffsets{0, 0, 0} {}

void LcdDisplay::begin() {
    static uint8_t downArrow[8] = {
        B00100,
        B00100,
        B00100,
        B00100,
        B11111,
        B01110,
        B00100,
        B00000
    };
    static uint8_t upArrow[8] = {
        B00100,
        B01110,
        B11111,
        B00100,
        B00100,
        B00100,
        B00100,
        B00000
    };

    _lcd.init();
    _lcd.createChar(kArrowDownCharSlot, downArrow);
    _lcd.createChar(kArrowUpCharSlot, upArrow);
    _lcd.backlight();
    _lcd.clear();
}

String LcdDisplay::buildDateString(const struct tm &localTime) const {
    char buffer[17];
    strftime(buffer, sizeof(buffer), "%a %d %b %Y", &localTime);
    return String(buffer);
}

String LcdDisplay::buildModeLabel(DisplayMode mode) const {
    switch (mode) {
        case DisplayMode::PH_DOWN_DOSE: {
            String label = "PH ";
            label += char(kArrowDownCharSlot);
            label += " DOSE";
            return label;
        }
        case DisplayMode::PH_DOWN_WAIT: {
            String label = "PH ";
            label += char(kArrowDownCharSlot);
            label += " WAIT";
            return label;
        }
        case DisplayMode::PH_UP_DOSE: {
            String label = "PH ";
            label += char(kArrowUpCharSlot);
            label += " DOSE";
            return label;
        }
        case DisplayMode::PH_UP_WAIT: {
            String label = "PH ";
            label += char(kArrowUpCharSlot);
            label += " WAIT";
            return label;
        }
        case DisplayMode::NUTRI_AB:
            return "NUTRI A+B";
        case DisplayMode::NUTRI_AB_WAIT:
            return "NUTRI A+B WAIT";
        case DisplayMode::NORMAL:
        default:
            return "NORMAL";
    }
}

String LcdDisplay::buildRightWindow(const String &text, uint8_t width, size_t offset) const {
    if (text.length() <= width) {
        return text;
    }

    String loopText = text;
    for (uint8_t i = 0; i < AppConfig::LCD_SCROLL_GAP_CHARS; i++) {
        loopText += ' ';
    }

    String window;
    window.reserve(width);

    for (uint8_t i = 0; i < width; i++) {
        const size_t charIndex = (offset + i) % loopText.length();
        window += loopText.charAt(charIndex);
    }

    return window;
}

String LcdDisplay::buildTimeString(const struct tm &localTime) const {
    char buffer[9];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", &localTime);
    return String(buffer);
}

void LcdDisplay::showMessage(const String &line1, const String &line2, const String &line3, const String &line4) {
    showStatusScreen(line1, line2, line3, line4);
}

void LcdDisplay::showInitializing() {
    showStatusScreen(
        "Hydroponic System",
        "Initializing...",
        "WiFi Connecting",
        "Please Wait"
    );
}

void LcdDisplay::showInitializingFinish() {
    showStatusScreen(
        "Hydroponic System",
        "Initializing",
        "Finish",
        "WiFi Connected"
    );
}

void LcdDisplay::showStatusScreen(const String &line1, const String &line2, const String &line3, const String &line4) {
    printCentered(_lcd, 0, line1, 20);
    printCentered(_lcd, 1, line2, 20);
    printCentered(_lcd, 2, line3, 20);
    printCentered(_lcd, 3, line4, 20);
}

void LcdDisplay::updateScrollState(const String rightTexts[], uint8_t count) {
    const unsigned long now = millis();
    if (now - _lastScrollMs < AppConfig::LCD_SCROLL_INTERVAL_MS) {
        for (uint8_t i = 0; i < count; i++) {
            if (rightTexts[i].length() <= kRightWidth) {
                _rightScrollOffsets[i] = 0;
            }
        }
        return;
    }

    _lastScrollMs = now;

    for (uint8_t i = 0; i < count; i++) {
        if (rightTexts[i].length() <= kRightWidth) {
            _rightScrollOffsets[i] = 0;
            continue;
        }

        const size_t scrollLength = rightTexts[i].length() + AppConfig::LCD_SCROLL_GAP_CHARS;
        _rightScrollOffsets[i] = (_rightScrollOffsets[i] + 1) % scrollLength;
    }
}

void LcdDisplay::printCentered(LiquidCrystal_I2C &lcd, uint8_t row, const String &text, uint8_t width) {
    String out = text;
    if (out.length() > width) {
        out = out.substring(0, width);
    }

    const uint8_t padding = (width - out.length()) / 2;
    lcd.setCursor(0, row);

    for (uint8_t i = 0; i < padding; i++) {
        lcd.print(' ');
    }

    lcd.print(out);

    for (uint8_t i = padding + out.length(); i < width; i++) {
        lcd.print(' ');
    }
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

void LcdDisplay::show(
    const SensorData &data,
    DisplayMode mode,
    bool wifiConnected,
    const struct tm *localTime,
    bool timeValid
) {
    const String leftTemp = "Temp:" + String(data.temperatureC, 1) + "C";
    const String leftTds = "TDS:" + String(data.tds, 0) + "ppm";
    const String leftPh = "pH:" + String(data.phValue, 2);

    printPadded(_lcd, 0, 0, leftTemp, kLeftWidth);
    printPadded(_lcd, 0, 1, leftTds, kLeftWidth);
    printPadded(_lcd, 0, 2, leftPh, kLeftWidth);

    String rightTexts[kRightLineCount];
    rightTexts[0] = "Hasna's Farm";

    if (timeValid && localTime != nullptr) {
        rightTexts[1] = buildDateString(*localTime);
        rightTexts[2] = buildTimeString(*localTime);
    } else if (wifiConnected) {
        rightTexts[1] = "Sync WIB";
        rightTexts[2] = "Wait NTP";
    } else {
        rightTexts[1] = "WiFi Off";
        rightTexts[2] = "No Time";
    }

    updateScrollState(rightTexts, kRightLineCount);

    for (uint8_t row = 0; row < kRightLineCount; row++) {
        printPadded(_lcd, kDividerColumn, row, "|", 1);
        const String window = buildRightWindow(rightTexts[row], kRightWidth, _rightScrollOffsets[row]);
        printPadded(_lcd, kRightColumn, row, window, kRightWidth);
    }

    const String modeLine = "Mode:" + buildModeLabel(mode);
    printPadded(_lcd, 0, 3, modeLine, 20);
}
