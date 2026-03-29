#include <Arduino.h>
#include <EEPROM.h>
#include <Wire.h>

#include "config/app_config.h"
#include "config/pins.h"
#include "control/sensor_manager.h"
#include "display/lcd_display.h"
#include "sensors/ph_sensor.h"
#include "sensors/tds_sensor.h"
#include "sensors/temp_sensor.h"

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

TempSensor tempSensor(Pins::TEMP_SENSOR, AppConfig::DEFAULT_WATER_TEMPERATURE_C);
SensorManager sensorManager(tdsSensor, phSensor, tempSensor, AppConfig::DEFAULT_WATER_TEMPERATURE_C);

LcdDisplay lcdDisplay(
    AppConfig::LCD_I2C_ADDRESS,
    AppConfig::LCD_COLUMNS,
    AppConfig::LCD_ROWS
);

void setup() {
    Serial.begin(115200);
    delay(1000);

    EEPROM.begin(64);

    Wire.begin(Pins::I2C_SDA, Pins::I2C_SCL);

    sensorManager.begin();
    lcdDisplay.begin();

    Serial.println();
}

void loop() {
    sensorManager.handleCalibrationSerial();
    sensorManager.update();

    static unsigned long lastPrintMs = 0;
    static unsigned long lastLcdMs = 0;
    const unsigned long now = millis();

    if (!sensorManager.isCalibrationMode() && (now - lastPrintMs >= AppConfig::SENSOR_PRINT_INTERVAL_MS)) {
        lastPrintMs = now;

        SensorData data = sensorManager.getSensorData();

        Serial.print("Temp: ");
        Serial.print(data.temperatureC, 2);
        Serial.print(" C | TDS: ");
        Serial.print(data.tds, 0);
        Serial.print(" ppm | pH: ");
        Serial.println(data.phValue, 2);
    }

    if (now - lastLcdMs >= AppConfig::LCD_REFRESH_INTERVAL_MS) {
        lastLcdMs = now;
        lcdDisplay.show(sensorManager.getSensorData(), sensorManager.getMode());
    }
}
