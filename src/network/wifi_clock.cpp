#include "network/wifi_clock.h"

#include <WiFi.h>

#include "config/app_config.h"

namespace {
constexpr int kMinValidYear = 2024;
}

WifiClock::WifiClock(const char *ssid, const char *password)
    : _ssid(ssid),
      _password(password),
      _timeConfigured(false),
      _lastWifiAttemptMs(0) {}

void WifiClock::begin() {
    WiFi.mode(WIFI_STA);
    WiFi.persistent(false);
    WiFi.setAutoReconnect(true);
    beginWifiConnection();
}

void WifiClock::update() {
    const unsigned long now = millis();

    if (WiFi.status() != WL_CONNECTED) {
        if (now - _lastWifiAttemptMs >= AppConfig::WIFI_RECONNECT_INTERVAL_MS) {
            beginWifiConnection();
        }
        return;
    }

    configureTimeIfNeeded();
}

bool WifiClock::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

bool WifiClock::getLocalTime(struct tm &timeInfo) const {
    const time_t now = time(nullptr);
    if (localtime_r(&now, &timeInfo) == nullptr) {
        return false;
    }

    return (timeInfo.tm_year + 1900) >= kMinValidYear;
}

void WifiClock::beginWifiConnection() {
    _lastWifiAttemptMs = millis();
    WiFi.begin(_ssid, _password);
}

void WifiClock::configureTimeIfNeeded() {
    if (_timeConfigured) {
        return;
    }

    configTime(
        AppConfig::WIB_UTC_OFFSET_SECONDS,
        0,
        AppConfig::NTP_SERVER_PRIMARY,
        AppConfig::NTP_SERVER_SECONDARY
    );

    _timeConfigured = true;
}
