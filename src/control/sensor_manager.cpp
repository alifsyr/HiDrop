#include "control/sensor_manager.h"

SensorManager::SensorManager(
    TdsSensor &tdsSensor,
    PhSensor &phSensor,
    TempSensor &tempSensor,
    float defaultTemperatureC
)
    : _tdsSensor(tdsSensor),
      _phSensor(phSensor),
      _tempSensor(tempSensor),
      _calibrationMode(false),
      _temperatureC(defaultTemperatureC),
      _phVoltage(0.0f),
      _phValue(7.0f),
      _tdsValue(0.0f),
      _lastTdsUpdateMs(0),
      _lastPhUpdateMs(0),
      _lastTdsReadMs(0),
      _mode(ReadMode::IDLE) {}

void SensorManager::begin() {
    _tdsSensor.begin();
    _phSensor.begin();
    _tempSensor.begin();

    _phVoltage = _phSensor.readVoltage();
    _phValue = _phSensor.convertVoltageToPh(_phVoltage);
    _lastPhUpdateMs = millis();
}

void SensorManager::handleCalibrationSerial() {
    if (!Serial.available()) return;

    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();

    if (cmd.length() == 0) return;

    if (cmd == "ENTER") {
        _calibrationMode = true;
        _mode = ReadMode::CALIBRATION;
    }

    _tdsSensor.calibrate(cmd.c_str());

    if (cmd == "EXIT") {
        _calibrationMode = false;
        _mode = ReadMode::IDLE;
    }
}

void SensorManager::update() {
    _tempSensor.update();
    _temperatureC = _tempSensor.getTemperatureC();

    if (_calibrationMode) {
        _mode = ReadMode::CALIBRATION;
        return;
    }

    const unsigned long now = millis();

    if (now - _lastTdsUpdateMs >= AppConfig::TDS_UPDATE_INTERVAL_MS) {
        _mode = ReadMode::TDS_READ;
        _tdsSensor.update(_temperatureC);
        _tdsValue = _tdsSensor.getTds();
        _lastTdsUpdateMs = now;
        _lastTdsReadMs = now;
        return;
    }

    const unsigned long sinceTds = now - _lastTdsReadMs;
    const bool quietWindowDone = sinceTds >= AppConfig::PH_QUIET_AFTER_TDS_MS;

    if (!quietWindowDone) {
        _mode = ReadMode::PH_QUIET;
        return;
    }

    if (now - _lastPhUpdateMs >= AppConfig::PH_UPDATE_INTERVAL_MS) {
        _mode = ReadMode::PH_READ;
        _phVoltage = _phSensor.readVoltage();
        float ph = _phSensor.convertVoltageToPh(_phVoltage);
        if (ph < 0.0f) ph = 0.0f;
        if (ph > 14.0f) ph = 14.0f;
        _phValue = ph;
        _lastPhUpdateMs = now;
        return;
    }

    _mode = ReadMode::IDLE;
}

bool SensorManager::isCalibrationMode() const {
    return _calibrationMode;
}

SensorData SensorManager::getSensorData() const {
    SensorData data;
    data.temperatureC = _temperatureC;
    data.tds = _tdsValue;
    data.phVoltage = _phVoltage;
    data.phValue = _phValue;
    return data;
}

float SensorManager::getPh() const {
    return _phValue;
}

float SensorManager::getTemperatureC() const {
    return _temperatureC;
}

const char *SensorManager::getMode() const {
    switch (_mode) {
        case ReadMode::TDS_READ:
            return "READING TDS";
        case ReadMode::PH_QUIET:
            return "QUIET BEFORE PH";
        case ReadMode::PH_READ:
            return "READING PH";
        case ReadMode::CALIBRATION:
            return "CALIBRATION";
        case ReadMode::IDLE:
        default:
            return "MONITOR";
    }
}
