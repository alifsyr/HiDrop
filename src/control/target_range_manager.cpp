#include "control/target_range_manager.h"

#include <EEPROM.h>
#include <math.h>
#include <stdlib.h>

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
        queueCurrentTargetsMessage();
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
        float minValue = 0.0f;
        float maxValue = 0.0f;
        if (!parseNumber(tokens[1], minValue) || !parseNumber(tokens[2], maxValue)) {
            Serial.println("Invalid pH number format. Example: SET PH 5.8 6.2");
            return true;
        }
        return setPhRange(minValue, maxValue);
    }

    if (count == 3 && tokens[0] == "PPM") {
        float minValue = 0.0f;
        float maxValue = 0.0f;
        if (!parseNumber(tokens[1], minValue) || !parseNumber(tokens[2], maxValue)) {
            Serial.println("Invalid PPM number format. Example: SET PPM 600 800");
            return true;
        }
        return setPpmRange(minValue, maxValue);
    }

    if (count == 4 && token0 == "SET" && tokens[1] == "PH") {
        float minValue = 0.0f;
        float maxValue = 0.0f;
        if (!parseNumber(tokens[2], minValue) || !parseNumber(tokens[3], maxValue)) {
            Serial.println("Invalid pH number format. Example: SET PH 5.8 6.2");
            return true;
        }
        return setPhRange(minValue, maxValue);
    }

    if (count == 4 && token0 == "SET" && tokens[1] == "PPM") {
        float minValue = 0.0f;
        float maxValue = 0.0f;
        if (!parseNumber(tokens[2], minValue) || !parseNumber(tokens[3], maxValue)) {
            Serial.println("Invalid PPM number format. Example: SET PPM 600 800");
            return true;
        }
        return setPpmRange(minValue, maxValue);
    }

    return false;
}

const TargetRanges &TargetRangeManager::getRanges() const {
    return _ranges;
}

bool TargetRangeManager::consumeDisplayMessage(String &line1, String &line2, String &line3, String &line4) {
    if (!_hasDisplayMessage) {
        return false;
    }

    line1 = _displayLine1;
    line2 = _displayLine2;
    line3 = _displayLine3;
    line4 = _displayLine4;
    _hasDisplayMessage = false;
    return true;
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

    if (!isValidRanges(stored.ranges)) {
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
    setDisplayMessage(
        "Targets Reset",
        "pH " + String(_ranges.phMin, 2) + " - " + String(_ranges.phMax, 2),
        "PPM " + String(_ranges.ppmMin, 0) + " - " + String(_ranges.ppmMax, 0),
        "Saved"
    );
}

bool TargetRangeManager::setPhRange(float minValue, float maxValue) {
    if (!isValidPhRange(minValue, maxValue)) {
        Serial.println("Invalid pH range. Example: SET PH 5.8 6.2");
        return true;
    }

    _ranges.phMin = minValue;
    _ranges.phMax = maxValue;
    saveToEeprom();
    setDisplayMessage(
        "pH Target Saved",
        String(_ranges.phMin, 2) + " - " + String(_ranges.phMax, 2),
        "",
        "Saved to EEPROM"
    );

    Serial.println("pH target updated.");
    printRanges();
    return true;
}

bool TargetRangeManager::setPpmRange(float minValue, float maxValue) {
    if (!isValidPpmRange(minValue, maxValue)) {
        Serial.println("Invalid PPM range. Example: SET PPM 600 800");
        return true;
    }

    _ranges.ppmMin = minValue;
    _ranges.ppmMax = maxValue;
    saveToEeprom();
    setDisplayMessage(
        "PPM Target Saved",
        String(_ranges.ppmMin, 0) + " - " + String(_ranges.ppmMax, 0),
        "",
        "Saved to EEPROM"
    );

    Serial.println("PPM target updated.");
    printRanges();
    return true;
}

void TargetRangeManager::queueCurrentTargetsMessage() {
    setDisplayMessage(
        "Current Targets",
        "pH " + String(_ranges.phMin, 2) + " - " + String(_ranges.phMax, 2),
        "PPM " + String(_ranges.ppmMin, 0) + " - " + String(_ranges.ppmMax, 0),
        "Active"
    );
}

void TargetRangeManager::setDisplayMessage(const String &line1, const String &line2, const String &line3, const String &line4) {
    _displayLine1 = line1;
    _displayLine2 = line2;
    _displayLine3 = line3;
    _displayLine4 = line4;
    _hasDisplayMessage = true;
}

bool TargetRangeManager::isValidPhRange(float minValue, float maxValue) {
    return isfinite(minValue) && isfinite(maxValue) && minValue >= 0.0f && maxValue <= 14.0f &&
           minValue < maxValue;
}

bool TargetRangeManager::isValidPpmRange(float minValue, float maxValue) {
    return isfinite(minValue) && isfinite(maxValue) && minValue >= 0.0f && maxValue <= 5000.0f &&
           minValue < maxValue;
}

bool TargetRangeManager::isValidRanges(const TargetRanges &ranges) {
    return isValidPhRange(ranges.phMin, ranges.phMax) &&
           isValidPpmRange(ranges.ppmMin, ranges.ppmMax);
}

bool TargetRangeManager::parseNumber(const String &token, float &outValue) {
    String normalized = token;
    normalized.trim();
    normalized.replace(",", ".");

    if (normalized.length() == 0) {
        return false;
    }

    const char *startPtr = normalized.c_str();
    char *endPtr = nullptr;
    const float parsed = strtof(startPtr, &endPtr);
    if (endPtr == startPtr || (endPtr != nullptr && *endPtr != '\0')) {
        return false;
    }

    if (!isfinite(parsed)) {
        return false;
    }

    outValue = parsed;
    return true;
}

uint8_t TargetRangeManager::tokenize(const String &input, String tokens[], uint8_t maxTokens) {
    String normalized = input;
    normalized.trim();
    normalized.toUpperCase();
    normalized.replace("=", " ");

    while (normalized.indexOf("  ") >= 0) {
        normalized.replace("  ", " ");
    }

    uint8_t count = 0;
    int start = 0;

    while (count < maxTokens && start < normalized.length()) {
        const int nextSpace = normalized.indexOf(' ', start);
        if (nextSpace < 0) {
            tokens[count++] = normalized.substring(start);
            break;
        }

        tokens[count++] = normalized.substring(start, nextSpace);
        start = nextSpace + 1;

        while (start < normalized.length() && normalized.charAt(start) == ' ') {
            start++;
        }
    }

    return count;
}
