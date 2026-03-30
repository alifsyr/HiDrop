#include <Arduino.h>
#include <EEPROM.h>
#include <Wire.h>
#include <WiFi.h>
#include <time.h>

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

bool connectWifiAndTime() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(AppConfig::WIFI_SSID, AppConfig::WIFI_PASSWORD);

    unsigned long startMs = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startMs < 15000) {
        delay(250);
        Serial.print('.');
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nWiFi gagal connect");
        return false;
    }

    Serial.println("\nWiFi connected");
    configTzTime(AppConfig::TZ_INFO, "pool.ntp.org", "time.nist.gov", "time.google.com");

    struct tm timeinfo;
    for (int i = 0; i < 20; i++) {
        if (getLocalTime(&timeinfo, 200)) {
            Serial.println("Waktu NTP siap (WIB)");
            return true;
        }
        delay(200);
    }

    Serial.println("NTP belum sync");
    return false;
}

String getWibDateText() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 10)) {
        return "Date offline";
    }

    char buff[24];
    strftime(buff, sizeof(buff), "%a %d %b %Y", &timeinfo);
    return String(buff);
}

String getWibTimeText() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 10)) {
        return "--:--:--";
    }

    char buff[16];
    strftime(buff, sizeof(buff), "%H:%M:%S", &timeinfo);
    return String(buff);
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    EEPROM.begin(64);

    Wire.begin(Pins::I2C_SDA, Pins::I2C_SCL);

    sensorManager.begin();
    lcdDisplay.begin();

    connectWifiAndTime();

    lcdDisplay.setMode(LcdDisplay::DosingMode::NORMAL);

    Serial.println();
}

void loop() {
    sensorManager.handleCalibrationSerial();
    sensorManager.update();

    if (sensorManager.isCalibrationMode()) {
        lcdDisplay.setMode(LcdDisplay::DosingMode::PH_DOWN_CAL);
    } else {
        lcdDisplay.setMode(LcdDisplay::DosingMode::NORMAL);
    }

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
        Serial.print(data.phValue, 2);
        Serial.print(" | Date: ");
        Serial.print(getWibDateText());
        Serial.print(" | Time: ");
        Serial.println(getWibTimeText());
    }

    if (now - lastLcdMs >= AppConfig::LCD_REFRESH_INTERVAL_MS) {
        lastLcdMs = now;
        lcdDisplay.show(
            sensorManager.getSensorData(),
            "Hasna's Farm",
            getWibDateText(),
            getWibTimeText()
        );
    }
}
