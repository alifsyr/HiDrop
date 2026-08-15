#pragma once

#include <Arduino.h>
#include <time.h>
#include <Firebase_ESP_Client.h>

#include "models/display_mode.h"
#include "models/dosing_report.h"
#include "models/sensor_data.h"
#include "models/target_ranges.h"

class FirebaseClient {
public:
    typedef std::function<bool(const String&)> CommandCallback;

    FirebaseClient();

    void begin();
    void update(
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
    );
    void handleClient();
    void addCompletedReport(const DosingReport &report);
    void setCommandCallback(CommandCallback cb);

private:
    static constexpr size_t kHistorySamplesSize = 48;

    struct Snapshot {
        SensorData sensorData = {};
        TargetRanges targets = {0.0f, 0.0f, 0.0f, 0.0f};
        const char *sensorMode = "MONITOR";
        bool calibrationMode = false;
        DisplayMode displayMode = DisplayMode::NORMAL;
        bool dosingBusy = false;
        const char *dosingState = "Monitoring";
        bool wifiConnected = false;
        bool timeValid = false;
        char date[16] = "N/A";
        char time[16] = "N/A";
        char ipAddress[16] = "0.0.0.0";
        unsigned long uptimeSeconds = 0;
    };

    struct HistorySample {
        uint16_t phX100 = 0;
        uint16_t ppm = 0;
    };

    FirebaseData _fbdo;
    FirebaseAuth _auth;
    FirebaseConfig _config;
    bool _firebaseReady;

    Snapshot _snapshot;
    HistorySample _historySamples[kHistorySamplesSize];
    size_t _historySampleCount;
    size_t _historySampleHead;
    unsigned long _lastHistorySampleMs;
    unsigned long _lastFirebaseUpdateMs;
    bool _wasWifiConnected;
    CommandCallback _commandCallback;
    OtaUpdater* _otaUpdater;

    void registerRoutes();
    void handleRoot();
    void handleStatus();
    void handleHistory();
    void handleReports();
    void handleSetTargetPh();
    void handleSetTargetPpm();
    void handleOtaCheck();
    void handleOtaStart();
    String buildHtmlPage() const;
    String buildStatusJson() const;
    String buildRecentReportsJson() const;
    String buildHistoryJson() const;
    void addHistorySample(const SensorData &sensorData);
    void sendStatusToFirebase();
    void sendHistoryToFirebase();
    void checkIncomingCommands();
    
    static void safeCopy(char *destination, size_t destinationSize, const char *source);
    static const char *displayModeLabel(DisplayMode mode);
    static uint16_t encodePhX100(float phValue);
    static uint16_t encodePpm(float ppmValue);
};
