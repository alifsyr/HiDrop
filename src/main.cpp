#include <Arduino.h>
#include <EEPROM.h>
#include <Wire.h>

#include "config/app_config.h"
#include "config/pins.h"
#include "control/dosing_controller.h"
#include "control/sensor_manager.h"
#include "control/target_range_manager.h"
#include "display/lcd_display.h"
#include "models/dosing_report.h"
#include "network/google_sheets_logger.h"
#include "network/wifi_clock.h"
#include "sensors/ph_sensor.h"
#include "sensors/tds_sensor.h"
#include "sensors/temp_sensor.h"

namespace {
bool wasWifiConnected = false;
unsigned long initFinishStartMs = 0;
unsigned long targetMessageUntilMs = 0;
String targetMessageLine1;
String targetMessageLine2;
String targetMessageLine3;
String targetMessageLine4;
}

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
WifiClock wifiClock(AppConfig::WIFI_SSID, AppConfig::WIFI_PASSWORD);
DosingController dosingController;
GoogleSheetsLogger sheetsLogger;
TargetRangeManager targetRangeManager;

void setup() {
    Serial.begin(115200);
    delay(1000);

    EEPROM.begin(64);

    Wire.begin(Pins::I2C_SDA, Pins::I2C_SCL);

    targetRangeManager.begin();
    sensorManager.begin();
    lcdDisplay.begin();
    wifiClock.begin();
    dosingController.begin();
    sheetsLogger.begin();

    Serial.println();
}

void loop() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();

        if (command.length() > 0 && !targetRangeManager.handleCommand(command)) {
            sensorManager.handleCalibrationCommand(command);
        }
    }

    if (targetRangeManager.consumeDisplayMessage(
        targetMessageLine1,
        targetMessageLine2,
        targetMessageLine3,
        targetMessageLine4
    )) {
        targetMessageUntilMs = millis() + AppConfig::LCD_TARGET_MESSAGE_DURATION_MS;
    }

    sensorManager.update();
    wifiClock.update();

    static unsigned long lastPrintMs = 0;
    static unsigned long lastLcdMs = 0;
    const unsigned long now = millis();
    const bool wifiConnected = wifiClock.isConnected();
    SensorData currentData = sensorManager.getSensorData();
    struct tm localTimeInfo = {};
    const bool timeValid = wifiClock.getLocalTime(localTimeInfo);

    if (!wifiConnected) {
        wasWifiConnected = false;
        initFinishStartMs = 0;
    } else if (!wasWifiConnected) {
        wasWifiConnected = true;
        initFinishStartMs = now;
    }

    dosingController.update(
        currentData,
        sensorManager.isCalibrationMode(),
        timeValid ? &localTimeInfo : nullptr,
        timeValid,
        targetRangeManager.getRanges()
    );

    DosingReport completedReport;
    if (dosingController.consumeCompletedReport(completedReport)) {
        sheetsLogger.queueReport(completedReport);
    }

    sheetsLogger.update(wifiConnected);

    if (!sensorManager.isCalibrationMode() && (now - lastPrintMs >= AppConfig::SENSOR_PRINT_INTERVAL_MS)) {
        lastPrintMs = now;

        Serial.print("Temp: ");
        Serial.print(currentData.temperatureC, 2);
        Serial.print(" C | TDS: ");
        Serial.print(currentData.tds, 0);
        Serial.print(" ppm | pH: ");
        Serial.println(currentData.phValue, 2);
    }

    if (now - lastLcdMs >= AppConfig::LCD_REFRESH_INTERVAL_MS) {
        lastLcdMs = now;

        if (now < targetMessageUntilMs) {
            lcdDisplay.showMessage(
                targetMessageLine1,
                targetMessageLine2,
                targetMessageLine3,
                targetMessageLine4
            );
            return;
        }

        if (!wifiConnected) {
            lcdDisplay.showInitializing();
            return;
        }

        if (initFinishStartMs != 0 && (now - initFinishStartMs) < AppConfig::LCD_INIT_FINISH_DURATION_MS) {
            lcdDisplay.showInitializingFinish();
            return;
        }

        lcdDisplay.show(
            currentData,
            dosingController.getDisplayMode(),
            wifiConnected,
            timeValid ? &localTimeInfo : nullptr,
            timeValid
        );
    }
}
