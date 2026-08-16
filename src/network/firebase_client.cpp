#include "network/firebase_client.h"

#include <WiFi.h>
// Provide the token generation process info.
#include <addons/TokenHelper.h>
// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

#include "config/app_config.h"

FirebaseClient::FirebaseClient()
    : _firebaseReady(false),
      _historySampleCount(0),
      _historySampleHead(0),
      _lastHistorySampleMs(0),
      _lastFirebaseUpdateMs(0),
      _wasWifiConnected(false) {}

void FirebaseClient::begin() {
    if (!AppConfig::FIREBASE_ENABLED || String(AppConfig::FIREBASE_HOST).length() == 0) {
        Serial.println("Firebase is disabled or credentials missing.");
        return;
    }

    Serial.println("Initializing Firebase...");
    _config.database_url = AppConfig::FIREBASE_HOST;
    _config.signer.tokens.legacy_token = AppConfig::FIREBASE_AUTH;

    // Assign the callback function for the long running token generation task
    _config.token_status_callback = tokenStatusCallback; 

    Firebase.begin(&_config, &_auth);
    Firebase.reconnectWiFi(true);
}

void FirebaseClient::setCommandCallback(CommandCallback cb) {
    _commandCallback = cb;
}

void FirebaseClient::setOtaTriggerCallback(OtaTriggerCallback cb) {
    _otaTriggerCallback = cb;
}

void FirebaseClient::reportOtaStatus(const String& status, int progress) {
    if (!AppConfig::FIREBASE_ENABLED || !Firebase.ready()) return;
    FirebaseJson json;
    json.set("ota_status", status);
    json.set("ota_progress", progress);
    Firebase.RTDB.updateNode(&_fbdo2, "/hydroponic/status/device", &json);
}

void FirebaseClient::update(
    const SensorData &sensorData,
    const TargetRanges &targetRanges,
    const char *sensorMode,
    bool calibrationMode,
    DisplayMode displayMode,
    bool dosingBusy,
    const char *dosingState,
    bool wifiConnected,
    const struct tm *localTime,
    bool timeValid
) {
    if (!AppConfig::FIREBASE_ENABLED) {
        return;
    }

    _snapshot.sensorData = sensorData;
    _snapshot.targets = targetRanges;
    _snapshot.sensorMode = sensorMode != nullptr ? sensorMode : "MONITOR";
    _snapshot.calibrationMode = calibrationMode;
    _snapshot.displayMode = displayMode;
    _snapshot.dosingBusy = dosingBusy;
    _snapshot.dosingState = dosingState != nullptr ? dosingState : "Monitoring";
    _snapshot.wifiConnected = wifiConnected;
    _snapshot.timeValid = timeValid;
    _snapshot.uptimeSeconds = millis() / 1000UL;

    safeCopy(
        _snapshot.ipAddress,
        sizeof(_snapshot.ipAddress),
        wifiConnected ? WiFi.localIP().toString().c_str() : "0.0.0.0"
    );

    if (timeValid && localTime != nullptr) {
        strftime(_snapshot.date, sizeof(_snapshot.date), "%Y-%m-%d", localTime);
        strftime(_snapshot.time, sizeof(_snapshot.time), "%H:%M:%S", localTime);
    } else {
        safeCopy(_snapshot.date, sizeof(_snapshot.date), "N/A");
        safeCopy(_snapshot.time, sizeof(_snapshot.time), "N/A");
    }

    const unsigned long now = millis();
    
    // History Sampling
    if (_historySampleCount == 0 ||
        (now - _lastHistorySampleMs) >= AppConfig::WEB_DASHBOARD_HISTORY_SAMPLE_INTERVAL_MS) {
        addHistorySample(sensorData);
        _lastHistorySampleMs = now;
        
        // Push history to Firebase when a new sample is generated
        if (Firebase.ready()) {
            sendHistoryToFirebase();
        }
    }

    // Status Update
    if (Firebase.ready() && (now - _lastFirebaseUpdateMs) >= AppConfig::FIREBASE_UPDATE_INTERVAL_MS) {
        sendStatusToFirebase();
        _lastFirebaseUpdateMs = now;
    }
    
    _wasWifiConnected = wifiConnected;
}

void FirebaseClient::handleClient() {
    if (!AppConfig::FIREBASE_ENABLED || !Firebase.ready()) {
        return;
    }

    checkIncomingCommands();
}

void FirebaseClient::addCompletedReport(const DosingReport &report) {
    if (!AppConfig::FIREBASE_ENABLED || !Firebase.ready()) {
        return;
    }

    FirebaseJson json;
    json.set("date", report.date);
    json.set("time", report.time);
    json.set("temperature_c", report.temperatureC);
    json.set("ppm_start", report.ppmStart);
    json.set("ppm_end", report.ppmEnd);
    json.set("na_ml", report.nutrientAMl);
    json.set("nb_ml", report.nutrientBMl);
    json.set("ph_start", report.phStart);
    json.set("ph_end", report.phEnd);
    json.set("ph_down_ml", report.phDownMl);
    json.set("ph_up_ml", report.phUpMl);
    json.set("manual_dilution_required", report.manualDilutionRequired);

    String path = "/hydroponic/reports";
    Firebase.RTDB.pushJSON(&_fbdo, path.c_str(), &json);
}

void FirebaseClient::addHistorySample(const SensorData &sensorData) {
    HistorySample &sample = _historySamples[_historySampleHead];
    sample.phX100 = encodePhX100(sensorData.phValue);
    sample.ppm = encodePpm(sensorData.tds);

    _historySampleHead = (_historySampleHead + 1) % kHistorySamplesSize;

    if (_historySampleCount < kHistorySamplesSize) {
        _historySampleCount++;
    }
}

void FirebaseClient::sendStatusToFirebase() {
    FirebaseJson json;
    
    json.set("device/firmware_version", FIRMWARE_VERSION);
    json.set("device/wifi_connected", _snapshot.wifiConnected);
    json.set("device/ip_address", _snapshot.ipAddress);
    json.set("device/time_valid", _snapshot.timeValid);
    json.set("device/date", _snapshot.date);
    json.set("device/time", _snapshot.time);
    json.set("device/uptime_seconds", _snapshot.uptimeSeconds);
    
    json.set("sensor/temperature_c", _snapshot.sensorData.temperatureC);
    json.set("sensor/ppm", _snapshot.sensorData.tds);
    json.set("sensor/ph_voltage", _snapshot.sensorData.phVoltage);
    json.set("sensor/ph", _snapshot.sensorData.phValue);
    json.set("sensor/mode", _snapshot.sensorMode);
    json.set("sensor/calibration_mode", _snapshot.calibrationMode);
    
    json.set("targets/ph_min", _snapshot.targets.phMin);
    json.set("targets/ph_max", _snapshot.targets.phMax);
    json.set("targets/ppm_min", _snapshot.targets.ppmMin);
    json.set("targets/ppm_max", _snapshot.targets.ppmMax);
    
    json.set("dosing/busy", _snapshot.dosingBusy);
    json.set("dosing/state", _snapshot.dosingState);
    json.set("dosing/display_mode", displayModeLabel(_snapshot.displayMode));
    
    // Include timestamp to allow frontend to know if data is fresh
    json.set("timestamp", millis()); // Or absolute unix time if timeValid

    Firebase.RTDB.setJSON(&_fbdo, "/hydroponic/status", &json);
}

void FirebaseClient::sendHistoryToFirebase() {
    FirebaseJson json;
    FirebaseJsonArray phArray;
    FirebaseJsonArray ppmArray;

    const size_t oldestIndex =
        (_historySampleHead + kHistorySamplesSize - _historySampleCount) % kHistorySamplesSize;

    for (size_t offset = 0; offset < _historySampleCount; ++offset) {
        const size_t historyIndex = (oldestIndex + offset) % kHistorySamplesSize;
        const HistorySample &sample = _historySamples[historyIndex];

        phArray.add(sample.phX100);
        ppmArray.add(sample.ppm);
    }

    json.set("sample_interval_ms", AppConfig::WEB_DASHBOARD_HISTORY_SAMPLE_INTERVAL_MS);
    json.set("ph_x100", phArray);
    json.set("ppm", ppmArray);

    Firebase.RTDB.setJSON(&_fbdo, "/hydroponic/history", &json);
}

void FirebaseClient::checkIncomingCommands() {
    // Basic polling mechanism to check if a command was sent from the web dashboard
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck < 2000) return;
    lastCheck = millis();

    // --- Check OTA trigger ---
    if (Firebase.RTDB.getBool(&_fbdo2, "/hydroponic/commands/ota/trigger")) {
        if (_fbdo2.boolData() == true) {
            // Read OTA details
            String otaUrl = "";
            String otaVersion = "";
            if (Firebase.RTDB.getString(&_fbdo2, "/hydroponic/commands/ota/url")) {
                otaUrl = _fbdo2.stringData();
            }
            if (Firebase.RTDB.getString(&_fbdo2, "/hydroponic/commands/ota/version")) {
                otaVersion = _fbdo2.stringData();
            }
            // Clear the trigger flag immediately to prevent re-trigger after reboot
            Firebase.RTDB.setBool(&_fbdo2, "/hydroponic/commands/ota/trigger", false);
            Serial.printf("[Firebase] OTA trigger received! URL: %s, Version: %s\n",
                          otaUrl.c_str(), otaVersion.c_str());
            if (_otaTriggerCallback && !otaUrl.isEmpty()) {
                _otaTriggerCallback(otaUrl, otaVersion);
            }
        }
    }

    // --- Check target update command ---
    if (Firebase.RTDB.getBool(&_fbdo, "/hydroponic/commands/targets/pending_update")) {
        if (_fbdo.boolData() == true) {
            // Read targets
            if (Firebase.RTDB.getJSON(&_fbdo, "/hydroponic/commands/targets")) {
                FirebaseJsonData jsonData;
                FirebaseJson &json = _fbdo.jsonObject();
                
                float ph_min = 0, ph_max = 0, ppm_min = 0, ppm_max = 0;
                
                json.get(jsonData, "ph_min");
                if (jsonData.success) ph_min = jsonData.doubleValue;
                
                json.get(jsonData, "ph_max");
                if (jsonData.success) ph_max = jsonData.doubleValue;
                
                json.get(jsonData, "ppm_min");
                if (jsonData.success) ppm_min = jsonData.doubleValue;
                
                json.get(jsonData, "ppm_max");
                if (jsonData.success) ppm_max = jsonData.doubleValue;
                
                if (_commandCallback) {
                    _commandCallback("SET PH " + String(ph_min) + " " + String(ph_max));
                    _commandCallback("SET PPM " + String(ppm_min) + " " + String(ppm_max));
                }
                
                // Clear the flag
                Firebase.RTDB.setBool(&_fbdo, "/hydroponic/commands/targets/pending_update", false);
            }
        }
    }
}

void FirebaseClient::safeCopy(char *destination, size_t destinationSize, const char *source) {
    if (destination == nullptr || destinationSize == 0) {
        return;
    }
    const char *safeSource = source != nullptr ? source : "";
    snprintf(destination, destinationSize, "%s", safeSource);
}

uint16_t FirebaseClient::encodePhX100(float phValue) {
    if (phValue <= 0.0f) return 0;
    if (phValue >= 14.0f) return 1400;
    return static_cast<uint16_t>((phValue * 100.0f) + 0.5f);
}

uint16_t FirebaseClient::encodePpm(float ppmValue) {
    if (ppmValue <= 0.0f) return 0;
    if (ppmValue >= 5000.0f) return 5000;
    return static_cast<uint16_t>(ppmValue + 0.5f);
}

const char *FirebaseClient::displayModeLabel(DisplayMode mode) {
    switch (mode) {
        case DisplayMode::PH_DOWN_DOSE: return "PH_DOWN_DOSE";
        case DisplayMode::PH_DOWN_WAIT: return "PH_DOWN_WAIT";
        case DisplayMode::PH_UP_DOSE: return "PH_UP_DOSE";
        case DisplayMode::PH_UP_WAIT: return "PH_UP_WAIT";
        case DisplayMode::NUTRI_AB: return "NUTRI_AB";
        case DisplayMode::NUTRI_AB_WAIT: return "NUTRI_AB_WAIT";
        case DisplayMode::NORMAL: default: return "NORMAL";
    }
}
