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
      _temperatureC(defaultTemperatureC) {}

void SensorManager::begin() {
    _tdsSensor.begin();
    _phSensor.begin();
    _tempSensor.begin();
}

void SensorManager::handleCalibrationSerial() {
    if (!Serial.available()) return;

    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();

    if (cmd.length() == 0) return;

    if (cmd == "ENTER") {
        _calibrationMode = true;
    }

    _tdsSensor.calibrate(cmd.c_str());

    if (cmd == "EXIT") {
        _calibrationMode = false;
    }
}

void SensorManager::update() {
    _tempSensor.update();
    _temperatureC = _tempSensor.getTemperatureC();

    _tdsSensor.update(_temperatureC);
}

bool SensorManager::isCalibrationMode() const {
    return _calibrationMode;
}

SensorData SensorManager::getTdsData() const {
    return _tdsSensor.getData(_temperatureC);
}

float SensorManager::getPh() const {
    return _phSensor.readPh();
}

float SensorManager::getTemperatureC() const {
    return _temperatureC;
}