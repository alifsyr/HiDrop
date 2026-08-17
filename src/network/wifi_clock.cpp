#include "network/wifi_clock.h"

#include <WiFi.h>

#include "config/app_config.h"
#include "utils/logger.h"

namespace {
constexpr int kMinValidYear = 2024;
// Globals for WiFiManager callback context
char g_staticIp[16] = "";
char g_staticGw[16] = "";
char g_staticSn[16] = "";
bool g_shouldSaveConfig = false;

void saveConfigCallback() {
    g_shouldSaveConfig = true;
}
}

WifiClock::WifiClock(const char *ssid, const char *password)
    : _ssid(ssid),
      _password(password),
      _timeConfigured(false) {
    memset(_staticIp, 0, sizeof(_staticIp));
    memset(_staticGw, 0, sizeof(_staticGw));
    memset(_staticSn, 0, sizeof(_staticSn));
}

void WifiClock::loadStaticIpConfig() {
    _prefs.begin("wifi_config", true);
    String ip = _prefs.getString("ip", "");
    String gw = _prefs.getString("gw", "");
    String sn = _prefs.getString("sn", "");
    _prefs.end();

    if (ip.length() > 0) strncpy(_staticIp, ip.c_str(), sizeof(_staticIp) - 1);
    if (gw.length() > 0) strncpy(_staticGw, gw.c_str(), sizeof(_staticGw) - 1);
    if (sn.length() > 0) strncpy(_staticSn, sn.c_str(), sizeof(_staticSn) - 1);
    
    // Copy to globals for callback access
    strncpy(g_staticIp, _staticIp, sizeof(g_staticIp));
    strncpy(g_staticGw, _staticGw, sizeof(g_staticGw));
    strncpy(g_staticSn, _staticSn, sizeof(g_staticSn));
}

void WifiClock::saveStaticIpConfig() {
    _prefs.begin("wifi_config", false);
    _prefs.putString("ip", g_staticIp);
    _prefs.putString("gw", g_staticGw);
    _prefs.putString("sn", g_staticSn);
    _prefs.end();
}

void WifiClock::setApCallback(std::function<void()> cb) {
    _apCallback = cb;
}

extern AsyncWebServer server;

void WifiClock::begin() {
    loadStaticIpConfig();
    
    _dns = new DNSServer();
    _wm = new AsyncWiFiManager(&server, _dns);

    if (_apCallback) {
        _wm->setAPCallback([this](AsyncWiFiManager *myWiFiManager) {
            _apCallback();
        });
    }

    _wm->setSaveConfigCallback(saveConfigCallback);

    // Custom parameters for Static IP
    _paramIp = new AsyncWiFiManagerParameter("ip", "Static IP (leave empty for DHCP)", _staticIp, 15);
    _paramGw = new AsyncWiFiManagerParameter("gw", "Gateway IP", _staticGw, 15);
    _paramSn = new AsyncWiFiManagerParameter("sn", "Subnet Mask", _staticSn, 15);

    _wm->addParameter(_paramIp);
    _wm->addParameter(_paramGw);
    _wm->addParameter(_paramSn);

    // Apply Static IP if configured
    if (strlen(_staticIp) > 0) {
        IPAddress ip, gw, sn;
        if (ip.fromString(_staticIp) && gw.fromString(_staticGw) && sn.fromString(_staticSn)) {
            _wm->setSTAStaticIPConfig(ip, gw, sn);
        }
    }

    // Use hardcoded SSID as fallback if no saved credentials (optional)
    if (_wm->autoConnect("Hydroponic_Setup")) {
        Logger::println("Connected to WiFi!");
    } else {
        Logger::println("WiFiManager started. Connect to AP 'Hydroponic_Setup' to configure.");
    }
}

void WifiClock::update() {
    // Check if we need to save custom parameters
    if (g_shouldSaveConfig) {
        if (_paramIp && _paramGw && _paramSn) {
            strncpy(g_staticIp, _paramIp->getValue(), sizeof(g_staticIp) - 1);
            strncpy(g_staticGw, _paramGw->getValue(), sizeof(g_staticGw) - 1);
            strncpy(g_staticSn, _paramSn->getValue(), sizeof(g_staticSn) - 1);
        }
        saveStaticIpConfig();
        g_shouldSaveConfig = false;
        
        // Reboot to apply new IP
        delay(1000);
        ESP.restart();
    }

    if (WiFi.status() != WL_CONNECTED) {
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

void WifiClock::resetSettings() {
    WiFi.disconnect(true, true);
    
    Preferences prefs;
    prefs.begin("wifi_config", false);
    prefs.clear();
    prefs.end();
    
    ESP.restart();
}
