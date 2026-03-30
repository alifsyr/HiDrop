#pragma once

#include <Arduino.h>

#include "models/dosing_report.h"

class GoogleSheetsLogger {
public:
    void begin();
    bool queueReport(const DosingReport &report);
    void update(bool wifiConnected);
    bool isConfigured() const;

private:
    static constexpr uint8_t kQueueSize = 5;

    DosingReport _queue[kQueueSize];
    uint8_t _head = 0;
    uint8_t _tail = 0;
    uint8_t _count = 0;
    unsigned long _lastAttemptMs = 0;

    String buildPayload(const DosingReport &report) const;
    void printFallback(const DosingReport &report) const;
    bool sendReport(const DosingReport &report);
};
