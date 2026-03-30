#pragma once

#include <Arduino.h>

#include "models/target_ranges.h"

class TargetRangeManager {
public:
    void begin();
    bool handleCommand(const String &command);
    const TargetRanges &getRanges() const;
    void printRanges() const;

private:
    TargetRanges _ranges;

    bool loadFromEeprom();
    bool saveToEeprom() const;
    void loadDefaults();
    void resetToDefaults();
    bool setPhRange(float minValue, float maxValue);
    bool setPpmRange(float minValue, float maxValue);
    static float parseNumber(String token);
    static uint8_t tokenize(String input, String tokens[], uint8_t maxTokens);
};
