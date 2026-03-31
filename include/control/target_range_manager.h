#pragma once

#include <Arduino.h>

#include "models/target_ranges.h"

class TargetRangeManager {
public:
    void begin();
    bool handleCommand(const String &command);
    bool consumeDisplayMessage(String &line1, String &line2, String &line3, String &line4);
    const TargetRanges &getRanges() const;
    void printRanges() const;

private:
    TargetRanges _ranges = {0.0f, 0.0f, 0.0f, 0.0f};
    String _displayLine1;
    String _displayLine2;
    String _displayLine3;
    String _displayLine4;
    bool _hasDisplayMessage = false;

    bool loadFromEeprom();
    bool saveToEeprom() const;
    void loadDefaults();
    void resetToDefaults();
    bool setPhRange(float minValue, float maxValue);
    bool setPpmRange(float minValue, float maxValue);
    static bool isValidPhRange(float minValue, float maxValue);
    static bool isValidPpmRange(float minValue, float maxValue);
    static bool isValidRanges(const TargetRanges &ranges);
    void queueCurrentTargetsMessage();
    void setDisplayMessage(const String &line1, const String &line2, const String &line3, const String &line4);
    static bool parseNumber(const String &token, float &outValue);
    static uint8_t tokenize(const String &input, String tokens[], uint8_t maxTokens);
};
