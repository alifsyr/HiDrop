#include "sensors/tds_sensor.h"

TdsSensor::TdsSensor(int pin, float aref, float adcRange, float smoothingAlpha)
    : _pin(pin),
      _aref(aref),
      _adcRange(adcRange),
      _lastTds(0.0f),
      _smoothingAlpha(smoothingAlpha),
      _initialized(false) {}

void TdsSensor::begin() {
    analogReadResolution(12);
    analogSetPinAttenuation(_pin, ADC_11db);

    _gravityTds.setPin(_pin);
    _gravityTds.setAref(_aref);
    _gravityTds.setAdcRange(_adcRange);
    _gravityTds.begin();
}

void TdsSensor::update(float temperatureC) {
    _gravityTds.setTemperature(temperatureC);
    _gravityTds.update();

    float rawTds = _gravityTds.getTdsValue();
    if (!_initialized) {
        _lastTds = rawTds;
        _initialized = true;
    } else {
        // Simple exponential smoothing to reduce noise/fluktuasi.
        _lastTds = (_smoothingAlpha * rawTds) + ((1.0f - _smoothingAlpha) * _lastTds);
    }
}

void TdsSensor::calibrate(const char* cmd) {
    _gravityTds.calibration(cmd);
}

float TdsSensor::getTds() const {
    return _lastTds;
}

float TdsSensor::getKvalue() {
    return _gravityTds.getKvalue();
}

SensorData TdsSensor::getData(float temperatureC) const {
    SensorData data;
    data.temperatureC = temperatureC;
    data.tds = _lastTds;
    return data;
}