#include <Arduino.h>
#include <EEPROM.h>
#include "config/pins.h"
#include "config/app_config.h"
#include "control/sensor_manager.h"
#include "sensors/tds_sensor.h"
#include "sensors/ph_sensor.h"

TdsSensor tdsSensor(
    Pins::TDS_SENSOR,
    AppConfig::ADC_VREF,
    AppConfig::ADC_RANGE,
    AppConfig::TDS_SMOOTHING_ALPHA
);

PhSensor phSensor(
    Pins::PH_SENSOR,
    AppConfig::ADC_VREF,
    AppConfig::ADC_RANGE,
    AppConfig::PH_SAMPLE_COUNT,
    AppConfig::PH_SLOPE,
    AppConfig::PH_CALIBRATION_VALUE
);

SensorManager sensorManager(tdsSensor, phSensor, AppConfig::DEFAULT_WATER_TEMPERATURE_C);

void setup() {
    Serial.begin(115200);
    delay(1000);

    EEPROM.begin(64);

    sensorManager.begin();
    Serial.println();
}

void loop() {
    sensorManager.handleCalibrationSerial();
    sensorManager.update();

    if (sensorManager.isCalibrationMode()) {
        delay(20);
        return;
    }

    static unsigned long lastPrintMs = 0;
    unsigned long now = millis();

    if (now - lastPrintMs >= AppConfig::SENSOR_PRINT_INTERVAL_MS) {
        lastPrintMs = now;

        SensorData data = sensorManager.getTdsData();
        float phValue = sensorManager.getPh();

        Serial.print("Temp: ");
        Serial.print(data.temperatureC, 2);
        Serial.print(" C | TDS: ");
        Serial.print(data.tds, 0);
        Serial.print(" ppm | pH: ");
        Serial.println(phValue, 2);
    }
}
