#include "sensors/temp_sensor.h"
#include "config/app_config.h"

TempSensor::TempSensor(uint8_t pin, float fallbackTemperatureC)
    : _pin(pin),
      _fallbackTemperatureC(fallbackTemperatureC),
      _temperatureC(fallbackTemperatureC),
      _valid(false),
      _lastRequestMs(0),
      _requestPending(false),
      _oneWire(pin),
      _dallas(&_oneWire) {}

void TempSensor::begin() {
    _dallas.begin();
    _dallas.setResolution(10);
    _dallas.setWaitForConversion(false);

    _dallas.requestTemperatures();
    _lastRequestMs = millis();
    _requestPending = true;
}

void TempSensor::update() {
    unsigned long now = millis();

    if (_requestPending) {
        if (now - _lastRequestMs >= AppConfig::TEMP_CONVERSION_DELAY_MS) {
            float tempC = _dallas.getTempCByIndex(0);

            if (tempC != DEVICE_DISCONNECTED_C && tempC > -55.0f && tempC < 125.0f) {
                _temperatureC = tempC;
                _valid = true;
            } else {
                _temperatureC = _fallbackTemperatureC;
                _valid = false;
            }

            _requestPending = false;
        }
    } else {
        if (now - _lastRequestMs >= AppConfig::TEMP_REQUEST_INTERVAL_MS) {
            _dallas.requestTemperatures();
            _lastRequestMs = now;
            _requestPending = true;
        }
    }
}

float TempSensor::getTemperatureC() const {
    return _temperatureC;
}

bool TempSensor::isValid() const {
    return _valid;
}