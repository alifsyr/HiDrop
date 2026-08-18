#pragma once

#include <Arduino.h>
#include <time.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <Preferences.h>
#include <functional>

#include <DNSServer.h>

class WifiClock {
public:
    WifiClock();
    ~WifiClock();

    void begin();
    void update();

    bool isConnected() const;
    bool getLocalTime(struct tm &timeInfo) const;
    
    // Call this to reset WiFi settings and reboot
    static void resetSettings();

    void setApCallback(std::function<void()> cb);

private:
    void configureTimeIfNeeded();
    void loadStaticIpConfig();
    void saveStaticIpConfig();

    bool _timeConfigured;
    
    AsyncWiFiManager* _wm = nullptr;
    DNSServer* _dns = nullptr;
    Preferences _prefs;

    char _staticIp[16];
    char _staticGw[16];
    char _staticSn[16];
    
    AsyncWiFiManagerParameter* _paramIp = nullptr;
    AsyncWiFiManagerParameter* _paramGw = nullptr;
    AsyncWiFiManagerParameter* _paramSn = nullptr;

    std::function<void()> _apCallback;
};
