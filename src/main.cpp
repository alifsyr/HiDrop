#include <Arduino.h>
#include <EEPROM.h>
#include <Wire.h>

#include "config/app_config.h"
#include "config/pins.h"
#include "utils/logger.h"
#include "control/dosing_controller.h"
#include "control/sensor_manager.h"
#include "control/target_range_manager.h"
#include "display/lcd_display.h"
#include "models/dosing_report.h"
#include "network/google_sheets_logger.h"
#include "network/ota_updater.h"
#include "network/firebase_client.h"
#include "network/wifi_clock.h"
#include "sensors/ph_sensor.h"
#include "sensors/tds_sensor.h"
#include "sensors/temp_sensor.h"

#include <ESPAsyncWebServer.h>
#include <WebSerial.h>

AsyncWebServer server(80);

namespace {
bool wasWifiConnected = false;
unsigned long initFinishStartMs = 0;
unsigned long targetMessageUntilMs = 0;
String targetMessageLine1;
String targetMessageLine2;
String targetMessageLine3;
String targetMessageLine4;
} // namespace

TdsSensor tdsSensor(Pins::TDS_SENSOR, AppConfig::ADC_VREF, AppConfig::ADC_RANGE,
                    AppConfig::TDS_SMOOTHING_ALPHA);

PhSensor phSensor(Pins::PH_SENSOR, AppConfig::ADC_VREF, AppConfig::ADC_RANGE,
                  AppConfig::PH_SAMPLE_COUNT, AppConfig::PH_SLOPE,
                  AppConfig::PH_CALIBRATION_VALUE);

TempSensor tempSensor(Pins::TEMP_SENSOR,
                      AppConfig::DEFAULT_WATER_TEMPERATURE_C);
SensorManager sensorManager(tdsSensor, phSensor, tempSensor,
                            AppConfig::DEFAULT_WATER_TEMPERATURE_C);

LcdDisplay lcdDisplay(AppConfig::LCD_I2C_ADDRESS, AppConfig::LCD_COLUMNS,
                      AppConfig::LCD_ROWS);
WifiClock wifiClock(AppConfig::WIFI_SSID, AppConfig::WIFI_PASSWORD);
DosingController dosingController;
GoogleSheetsLogger sheetsLogger;
FirebaseClient firebaseClient;
TargetRangeManager targetRangeManager;
OtaUpdater otaUpdater("alifsyr", "HiDrop");

void processCommand(String command) {
  command.trim();
  if (command.length() == 0) return;
  
  WebSerial.println("> " + command);
  Serial.println("> " + command);

  SensorData currentCmdData = sensorManager.getSensorData();
  struct tm localTimeCmdInfo = {};
  bool timeValidCmd = wifiClock.getLocalTime(localTimeCmdInfo);

  if (command == "RESET WIFI") {
      Logger::println("Resetting WiFi config and rebooting...");
      delay(1000);
      WifiClock::resetSettings();
      return;
  }

  if (!dosingController.triggerManualDose(
          command, currentCmdData,
          timeValidCmd ? &localTimeCmdInfo : nullptr, timeValidCmd) &&
      !targetRangeManager.handleCommand(command)) {
    sensorManager.handleCalibrationCommand(command);
  }
}

void recvMsg(uint8_t *data, size_t len) {
  String cmd = "";
  for(size_t i=0; i < len; i++){
    cmd += char(data[i]);
  }
  processCommand(cmd);
}

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
  firebaseClient.setCommandCallback(
      [](const String &cmd) { return targetRangeManager.handleCommand(cmd); });
  firebaseClient.setOtaTriggerCallback(
      [](const String &url, const String &version) {
          Serial.printf("[Main] OTA update triggered! Version: %s\n", version.c_str());
          firebaseClient.reportOtaStatus("downloading", 0);
          bool ok = otaUpdater.startUpdateFromUrl(url, [](int pct) {
              firebaseClient.reportOtaStatus("downloading", pct);
          });
          if (!ok) {
              Serial.println("[Main] OTA update failed!");
              firebaseClient.reportOtaStatus("failed", 0);
          }
          // If ok, ESP32 auto-restarts. reportOtaStatus("success") is not needed
          // because the new firmware will overwrite the status on next boot.
      });
  otaUpdater.begin();
  firebaseClient.begin();

  // Initialize WebSerial
  WebSerial.begin(&server);
  WebSerial.onMessage(recvMsg);
  server.begin();

  Serial.println();

#if DEV_MODE
  Serial.println("╔══════════════════════════════════════╗");
  Serial.println("║        *** DEVELOPMENT MODE ***       ║");
  Serial.println("║  Auto-dosing DISABLED                 ║");
  Serial.println("║  Type 'DEV STATUS' for commands       ║");
  Serial.println("╚══════════════════════════════════════╝");
#endif
}

void loop() {
  static char serialCommandBuffer[96];
  static size_t serialCommandLength = 0;

  while (Serial.available() > 0) {
    const char incoming = static_cast<char>(Serial.read());

    if (incoming == '\r') {
      continue;
    }

    if (incoming == '\n') {
      if (serialCommandLength == 0) {
        continue;
      }

      serialCommandBuffer[serialCommandLength] = '\0';
      String command(serialCommandBuffer);
      command.trim();

      if (command.length() > 0) {
        processCommand(command);
      }

      serialCommandLength = 0;
      continue;
    }

    if (serialCommandLength < (sizeof(serialCommandBuffer) - 1)) {
      serialCommandBuffer[serialCommandLength++] = incoming;
    } else {
      serialCommandLength = 0;
      Serial.println("Serial command too long. Max 95 chars.");
    }
  }

  if (targetRangeManager.consumeDisplayMessage(
          targetMessageLine1, targetMessageLine2, targetMessageLine3,
          targetMessageLine4)) {
    targetMessageUntilMs = millis() + AppConfig::LCD_TARGET_MESSAGE_DURATION_MS;
  }

  sensorManager.update();
  wifiClock.update();
  otaUpdater.update();

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
    Serial.print("WiFi Connected! IP Address: ");
    Serial.println(WiFi.localIP());
    WebSerial.print("WiFi Connected! IP Address: ");
    WebSerial.println(WiFi.localIP());
  }

  dosingController.update(currentData, sensorManager.isCalibrationMode(),
                          timeValid ? &localTimeInfo : nullptr, timeValid,
                          targetRangeManager.getRanges());

  DosingReport completedReport;
  if (dosingController.consumeCompletedReport(completedReport)) {
    firebaseClient.addCompletedReport(completedReport);
    sheetsLogger.queueReport(completedReport);
  }

  sheetsLogger.update(wifiConnected);
  firebaseClient.update(
      currentData, targetRangeManager.getRanges(), sensorManager.getMode(),
      sensorManager.isCalibrationMode(), dosingController.getDisplayMode(),
      dosingController.isBusy(), dosingController.getStateLabel(),
      wifiConnected, timeValid ? &localTimeInfo : nullptr, timeValid);
  firebaseClient.handleClient();

  if (!sensorManager.isCalibrationMode() &&
      (now - lastPrintMs >= 10000)) { // 10 seconds heartbeat
    lastPrintMs = now;

    unsigned long uptimeSec = now / 1000;
    unsigned long hours = uptimeSec / 3600;
    unsigned long mins = (uptimeSec % 3600) / 60;
    unsigned long secs = uptimeSec % 60;

    char logBuf[128];
    snprintf(logBuf, sizeof(logBuf), "[Status%s] Uptime: %02lu:%02lu:%02lu | Temp: %.1fC | TDS: %.0fppm | pH: %.2f | State: %s",
        dosingController.isDevMode() ? "|DEV" : "",
        hours, mins, secs,
        currentData.temperatureC, currentData.tds, currentData.phValue, 
        dosingController.getStateLabel());
        
    Serial.println(logBuf);
    WebSerial.println(logBuf);
  }

  if (now - lastLcdMs >= AppConfig::LCD_REFRESH_INTERVAL_MS) {
    lastLcdMs = now;

    if (now < targetMessageUntilMs) {
      lcdDisplay.showMessage(targetMessageLine1, targetMessageLine2,
                             targetMessageLine3, targetMessageLine4);
      return;
    }

    if (initFinishStartMs != 0 &&
        (now - initFinishStartMs) < AppConfig::LCD_INIT_FINISH_DURATION_MS) {
      lcdDisplay.showInitializingFinish();
      return;
    }

    lcdDisplay.show(currentData, dosingController.getDisplayMode(),
                    wifiConnected, timeValid ? &localTimeInfo : nullptr,
                    timeValid);
  }
}
