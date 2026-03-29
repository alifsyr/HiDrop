#pragma once

#include "config/app_config.h"
#include "models/sensor_data.h"
#include "sensors/ph_sensor.h"
#include "sensors/tds_sensor.h"
#include "sensors/temp_sensor.h"

class SensorManager {
public:
    SensorManager(
        TdsSensor &tdsSensor,
        PhSensor &phSensor,
        TempSensor &tempSensor,
        float defaultTemperatureC = AppConfig::DEFAULT_WATER_TEMPERATURE_C
    );

    void begin();
    void handleCalibrationSerial();
    void update();

    bool isCalibrationMode() const;
    SensorData getTdsData() const;
    float getPh() const;
    float getTemperatureC() const;

private:
    TdsSensor &_tdsSensor;
    PhSensor &_phSensor;
    TempSensor &_tempSensor;

    bool _calibrationMode;
    float _temperatureC;
};