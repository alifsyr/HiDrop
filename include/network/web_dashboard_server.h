#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include <time.h>

#include "models/display_mode.h"
#include "models/dosing_report.h"
#include "models/sensor_data.h"
#include "models/target_ranges.h"

class WebDashboardServer {
public:
    WebDashboardServer();

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

private:
    static constexpr size_t kRecentReportsSize = 6;
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

    WebServer _server;
    Snapshot _snapshot;
    DosingReport _recentReports[kRecentReportsSize];
    HistorySample _historySamples[kHistorySamplesSize];
    size_t _recentReportCount;
    size_t _recentReportHead;
    size_t _historySampleCount;
    size_t _historySampleHead;
    unsigned long _lastHistorySampleMs;
    bool _wasWifiConnected;

    void registerRoutes();
    void handleRoot();
    void handleStatus();
    void handleHistory();
    void handleReports();
    String buildHtmlPage() const;
    String buildStatusJson() const;
    String buildRecentReportsJson() const;
    String buildHistoryJson() const;
    void addHistorySample(const SensorData &sensorData);
    static void safeCopy(char *destination, size_t destinationSize, const char *source);
    static String escapeJson(const String &value);
    static const char *displayModeLabel(DisplayMode mode);
    static uint16_t encodePhX100(float phValue);
    static uint16_t encodePpm(float ppmValue);
};
