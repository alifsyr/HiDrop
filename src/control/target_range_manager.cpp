#include "control/target_range_manager.h"

#include <EEPROM.h>

#include "config/app_config.h"

namespace {
constexpr uint32_t kTargetRangeMagic = 0x48524431UL;
constexpr int kTargetRangeEepromAddress = 32; // avoid GravityTDS default EEPROM address 0x08

struct StoredTargetRanges {
    uint32_t magic;
    TargetRanges ranges;
};
}

void TargetRangeManager::begin() {
    if (!loadFromEeprom()) {
        resetToDefaults();
    } else {
        printRanges();
    }
}

bool TargetRangeManager::handleCommand(const String &command) {
    String input = command;
    input.trim();
    if (input.length() == 0) {
        return false;
    }

    String tokens[4];
    const uint8_t count = tokenize(input, tokens, 4);
    if (count == 0) {
        return false;
    }

    const String token0 = tokens[0];

    if ((count == 1 && (token0 == "TARGETS" || token0 == "RANGES")) ||
        (count == 2 && token0 == "SHOW" && (tokens[1] == "TARGETS" || tokens[1] == "RANGES"))) {
        printRanges();
        return true;
    }

    if (count == 2 && token0 == "RESET" && (tokens[1] == "TARGETS" || tokens[1] == "RANGES")) {
        resetToDefaults();
        Serial.println("Target ranges reset to defaults.");
        printRanges();
        return true;
    }

    if (count == 3 && tokens[0] == "PH") {
        return setPhRange(parseNumber(tokens[1]), parseNumber(tokens[2]));
    }

    if (count == 3 && tokens[0] == "PPM") {
        return setPpmRange(parseNumber(tokens[1]), parseNumber(tokens[2]));
    }

    if (count == 4 && token0 == "SET" && tokens[1] == "PH") {
        return setPhRange(parseNumber(tokens[2]), parseNumber(tokens[3]));
    }

    if (count == 4 && token0 == "SET" && tokens[1] == "PPM") {
        return setPpmRange(parseNumber(tokens[2]), parseNumber(tokens[3]));
    }

    return false;
}

const TargetRanges &TargetRangeManager::getRanges() const {
    return _ranges;
}

void TargetRangeManager::printRanges() const {
    Serial.println("Current target ranges:");
    Serial.print("- pH : ");
    Serial.print(_ranges.phMin, 2);
    Serial.print(" - ");
    Serial.println(_ranges.phMax, 2);
    Serial.print("- PPM: ");
    Serial.print(_ranges.ppmMin, 0);
    Serial.print(" - ");
    Serial.println(_ranges.ppmMax, 0);
    Serial.println("Commands:");
    Serial.println("  SET PH <min> <max>");
    Serial.println("  SET PPM <min> <max>");
    Serial.println("  SHOW TARGETS");
    Serial.println("  RESET TARGETS");
}

bool TargetRangeManager::loadFromEeprom() {
    StoredTargetRanges stored = {};
    EEPROM.get(kTargetRangeEepromAddress, stored);

    if (stored.magic != kTargetRangeMagic) {
        return false;
    }

    _ranges = stored.ranges;
    return true;
}

bool TargetRangeManager::saveToEeprom() const {
    StoredTargetRanges stored = {};
    stored.magic = kTargetRangeMagic;
    stored.ranges = _ranges;
    EEPROM.put(kTargetRangeEepromAddress, stored);
    return EEPROM.commit();
}

void TargetRangeManager::loadDefaults() {
    _ranges.phMin = AppConfig::DEFAULT_PH_TARGET_MIN;
    _ranges.phMax = AppConfig::DEFAULT_PH_TARGET_MAX;
    _ranges.ppmMin = AppConfig::DEFAULT_PPM_TARGET_MIN;
    _ranges.ppmMax = AppConfig::DEFAULT_PPM_TARGET_MAX;
}

void TargetRangeManager::resetToDefaults() {
    loadDefaults();
    saveToEeprom();
}

bool TargetRangeManager::setPhRange(float minValue, float maxValue) {
    if (minValue < 0.0f || maxValue > 14.0f || minValue >= maxValue) {
        Serial.println("Invalid pH range. Example: SET PH 5.8 6.2");
        return true;
    }

    _ranges.phMin = minValue;
    _ranges.phMax = maxValue;
    saveToEeprom();

    Serial.println("pH target updated.");
    printRanges();
    return true;
}

bool TargetRangeManager::setPpmRange(float minValue, float maxValue) {
    if (minValue < 0.0f || maxValue > 5000.0f || minValue >= maxValue) {
        Serial.println("Invalid PPM range. Example: SET PPM 600 800");
        return true;
    }

    _ranges.ppmMin = minValue;
    _ranges.ppmMax = maxValue;
    saveToEeprom();

    Serial.println("PPM target updated.");
    printRanges();
    return true;
}

float TargetRangeManager::parseNumber(String token) {
    token.trim();
    token.replace(",", ".");
    return token.toFloat();
}

uint8_t TargetRangeManager::tokenize(String input, String tokens[], uint8_t maxTokens) {
    input.trim();
    input.toUpperCase();
    input.replace("=", " ");

    while (input.indexOf("  ") >= 0) {
        input.replace("  ", " ");
    }

    uint8_t count = 0;
    int start = 0;

    while (count < maxTokens && start < input.length()) {
        const int nextSpace = input.indexOf(' ', start);
        if (nextSpace < 0) {
            tokens[count++] = input.substring(start);
            break;
        }

        tokens[count++] = input.substring(start, nextSpace);
        start = nextSpace + 1;

        while (start < input.length() && input.charAt(start) == ' ') {
            start++;
        }
    }

    return count;
}
