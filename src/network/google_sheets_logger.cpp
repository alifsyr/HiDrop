#include "network/google_sheets_logger.h"

#include <HTTPClient.h>

#include "config/app_config.h"

void GoogleSheetsLogger::begin() {
    _head = 0;
    _tail = 0;
    _count = 0;
    _lastAttemptMs = 0;
}

bool GoogleSheetsLogger::queueReport(const DosingReport &report) {
    if (!AppConfig::GOOGLE_SHEETS_LOGGING_ENABLED || !isConfigured()) {
        printFallback(report);
        return false;
    }

    if (_count >= kQueueSize) {
        Serial.println("Sheets logger queue full, dropping oldest report.");
        _head = (_head + 1) % kQueueSize;
        _count--;
    }

    _queue[_tail] = report;
    _tail = (_tail + 1) % kQueueSize;
    _count++;
    return true;
}

void GoogleSheetsLogger::update(bool wifiConnected) {
    if (!AppConfig::GOOGLE_SHEETS_LOGGING_ENABLED || !isConfigured()) {
        return;
    }

    if (!wifiConnected || _count == 0) {
        return;
    }

    const unsigned long now = millis();
    if (now - _lastAttemptMs < AppConfig::GOOGLE_SHEETS_RETRY_INTERVAL_MS && _lastAttemptMs != 0) {
        return;
    }

    _lastAttemptMs = now;

    if (!sendReport(_queue[_head])) {
        return;
    }

    _head = (_head + 1) % kQueueSize;
    _count--;
}

bool GoogleSheetsLogger::isConfigured() const {
    return AppConfig::GOOGLE_SHEETS_WEB_APP_URL[0] != '\0';
}

String GoogleSheetsLogger::buildPayload(const DosingReport &report) const {
    String payload = "{";
    payload += "\"date\":\"" + report.date + "\",";
    payload += "\"time\":\"" + report.time + "\",";
    payload += "\"temp\":" + String(report.temperatureC, 2) + ",";
    payload += "\"ppm_start\":" + String(report.ppmStart, 0) + ",";
    payload += "\"ppm_end\":" + String(report.ppmEnd, 0) + ",";
    payload += "\"na_ml\":" + String(report.nutrientAMl, 2) + ",";
    payload += "\"nb_ml\":" + String(report.nutrientBMl, 2) + ",";
    payload += "\"ph_start\":" + String(report.phStart, 2) + ",";
    payload += "\"ph_end\":" + String(report.phEnd, 2) + ",";
    payload += "\"ph_down_ml\":" + String(report.phDownMl, 2) + ",";
    payload += "\"ph_up_ml\":" + String(report.phUpMl, 2) + ",";
    payload += "\"manual_dilution_required\":";
    payload += report.manualDilutionRequired ? "true" : "false";

    if (AppConfig::GOOGLE_SHEETS_SHARED_SECRET[0] != '\0') {
        payload += ",\"token\":\"";
        payload += AppConfig::GOOGLE_SHEETS_SHARED_SECRET;
        payload += "\"";
    }

    payload += "}";
    return payload;
}

void GoogleSheetsLogger::printFallback(const DosingReport &report) const {
    Serial.println("Google Sheets logging disabled or URL not configured.");
    Serial.print("Report -> Date: ");
    Serial.print(report.date);
    Serial.print(" Time: ");
    Serial.print(report.time);
    Serial.print(" Temp: ");
    Serial.print(report.temperatureC, 2);
    Serial.print(" PPM Start: ");
    Serial.print(report.ppmStart, 0);
    Serial.print(" PPM End: ");
    Serial.print(report.ppmEnd, 0);
    Serial.print(" NA: ");
    Serial.print(report.nutrientAMl, 2);
    Serial.print(" NB: ");
    Serial.print(report.nutrientBMl, 2);
    Serial.print(" pH Start: ");
    Serial.print(report.phStart, 2);
    Serial.print(" pH End: ");
    Serial.print(report.phEnd, 2);
    Serial.print(" pH D: ");
    Serial.print(report.phDownMl, 2);
    Serial.print(" pH U: ");
    Serial.print(report.phUpMl, 2);
    Serial.print(" Manual Dilution: ");
    Serial.println(report.manualDilutionRequired ? "YES" : "NO");
}

bool GoogleSheetsLogger::sendReport(const DosingReport &report) {
    HTTPClient http;
    http.setTimeout(AppConfig::GOOGLE_SHEETS_TIMEOUT_MS);

    if (!http.begin(AppConfig::GOOGLE_SHEETS_WEB_APP_URL)) {
        Serial.println("Failed to start HTTP session for Google Sheets.");
        return false;
    }

    http.addHeader("Content-Type", "application/json");
    const int httpCode = http.POST(buildPayload(report));

    if (httpCode < 200 || httpCode >= 300) {
        Serial.print("Google Sheets POST failed with code: ");
        Serial.println(httpCode);
        http.end();
        return false;
    }

    Serial.println("Google Sheets report sent.");
    http.end();
    return true;
}
