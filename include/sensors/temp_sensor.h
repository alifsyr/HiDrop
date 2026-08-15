#pragma once

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

class TempSensor {
public:
    explicit TempSensor(uint8_t pin, float fallbackTemperatureC = 25.0f);

    void begin();
    void update();

    float getTemperatureC() const;
    bool isValid() const;

private:
    uint8_t _pin;
    float _fallbackTemperatureC;
    float _temperatureC;
    bool _valid;

    unsigned long _lastRequestMs;
    bool _requestPending;

    OneWire _oneWire;
    DallasTemperature _dallas;
};
