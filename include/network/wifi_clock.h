#pragma once

#include <Arduino.h>
#include <time.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <Preferences.h>

#include <DNSServer.h>

class WifiClock {
public:
    WifiClock(const char *ssid, const char *password);

    void begin();
    void update();

    bool isConnected() const;
    bool getLocalTime(struct tm &timeInfo) const;
    
    // Call this to reset WiFi settings and reboot
    static void resetSettings();

private:
    void configureTimeIfNeeded();
    void loadStaticIpConfig();
    void saveStaticIpConfig();

    const char *_ssid;
    const char *_password;
    bool _timeConfigured;
    
    AsyncWiFiManager* _wm;
    DNSServer* _dns;
    Preferences _prefs;

    char _staticIp[16];
    char _staticGw[16];
    char _staticSn[16];
    
    AsyncWiFiManagerParameter* _paramIp;
    AsyncWiFiManagerParameter* _paramGw;
    AsyncWiFiManagerParameter* _paramSn;
};
