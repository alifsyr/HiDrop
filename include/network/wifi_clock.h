#pragma once

#include <Arduino.h>
#include <time.h>

class WifiClock {
public:
    WifiClock(const char *ssid, const char *password);

    void begin();
    void update();

    bool isConnected() const;
    bool getLocalTime(struct tm &timeInfo) const;

private:
    void beginWifiConnection();
    void configureTimeIfNeeded();

    const char *_ssid;
    const char *_password;
    bool _timeConfigured;
    unsigned long _lastWifiAttemptMs;
};
