#pragma once

#include <Arduino.h>

class PhSensor {
public:
    PhSensor(int pin, float vref, float adcRange, int sampleCount, float slope, float calibrationValue);

    void begin();
    float readVoltage();
    float readPh();

private:
    int _pin;
    float _vref;
    float _adcRange;
    int _sampleCount;
    float _slope;
    float _calibrationValue;

    int getMedianAverageRaw();
};