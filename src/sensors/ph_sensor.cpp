#include "sensors/ph_sensor.h"

PhSensor::PhSensor(int pin, float vref, float adcRange, int sampleCount, float slope, float calibrationValue)
    : _pin(pin),
      _vref(vref),
      _adcRange(adcRange),
      _sampleCount(sampleCount),
      _slope(slope),
      _calibrationValue(calibrationValue) {}

void PhSensor::begin() {
    analogReadResolution(12);
    analogSetPinAttenuation(_pin, ADC_11db);
    pinMode(_pin, INPUT);
}

int PhSensor::getMedianAverageRaw() {
    int buffer[20];

    if (_sampleCount > 20) {
        return analogRead(_pin);
    }

    for (int i = 0; i < _sampleCount; i++) {
        buffer[i] = analogRead(_pin);
        delay(30);
    }

    for (int i = 0; i < _sampleCount - 1; i++) {
        for (int j = i + 1; j < _sampleCount; j++) {
            if (buffer[i] > buffer[j]) {
                int temp = buffer[i];
                buffer[i] = buffer[j];
                buffer[j] = temp;
            }
        }
    }

    unsigned long sum = 0;
    int start = 2;
    int end = _sampleCount - 2;

    for (int i = start; i < end; i++) {
        sum += buffer[i];
    }

    int count = end - start;
    if (count <= 0) {
        return analogRead(_pin);
    }

    return sum / count;
}

float PhSensor::readVoltage() {
    int avgRaw = getMedianAverageRaw();
    return avgRaw * (_vref / _adcRange);
}

float PhSensor::convertVoltageToPh(float voltage) const {
    return (_slope * voltage) + _calibrationValue;
}

float PhSensor::readPh() {
    float volt = readVoltage();
    return convertVoltageToPh(volt);
}
