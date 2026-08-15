#include "network/ota_updater.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <ArduinoJson.h>

OtaUpdater::OtaUpdater(const String& repoOwner, const String& repoName) 
    : _repoOwner(repoOwner), _repoName(repoName), 
      _updateAvailable(false), _lastCheckMs(0) {}

void OtaUpdater::begin() {
    _updateAvailable = false;
    _latestVersion = FIRMWARE_VERSION;
}

void OtaUpdater::update(bool forceCheck) {
    if (!WiFi.isConnected()) return;

    unsigned long now = millis();
    if (now == 0) now = 1;

    // Check every 12 hours (43200000 ms)
    if (_lastCheckMs == 0) {
        _lastCheckMs = now; // Initialize the timer on first connect
    }

    // Wait 10 seconds after boot/connect before doing the first OTA check 
    // to avoid blocking the main loop while LCD is showing "WiFi connecting".
    if (!forceCheck && (now - _lastCheckMs < 10000)) {
        return;
    }

    if (forceCheck || (now - _lastCheckMs) > 43200000UL) {
        _lastCheckMs = now;
        checkForUpdate();
    }
}

bool OtaUpdater::checkForUpdate() {
    Serial.println("[OTA] Checking for updates from GitHub...");
    
    // Allocate heavily used TLS objects on the heap instead of the stack
    // because ESP32's loop task only has 8KB stack by default, which can cause a crash.
    WiFiClientSecure* client = new WiFiClientSecure();
    if (!client) {
        Serial.println("[OTA] Failed to allocate WiFiClientSecure");
        return false;
    }
    client->setInsecure(); // GitHub API uses HTTPS. For simplicity, we bypass cert validation.
    
    HTTPClient* http = new HTTPClient();
    if (!http) {
        Serial.println("[OTA] Failed to allocate HTTPClient");
        delete client;
        return false;
    }
    
    String url = "https://api.github.com/repos/" + _repoOwner + "/" + _repoName + "/releases/latest";
    http->begin(*client, url);
    http->addHeader("User-Agent", "ESP32-OtaUpdater");
    
    int httpCode = http->GET();
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("[OTA] Failed to check update. HTTP Code: %d\n", httpCode);
        http->end();
        delete http;
        delete client;
        return false;
    }
    
    String payload = http->getString();
    http->end();
    delete http;
    delete client;
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
        Serial.printf("[OTA] JSON Parse Failed: %s\n", error.c_str());
        return false;
    }
    
    String tagName = doc["tag_name"] | "";
    if (tagName.startsWith("v")) {
        tagName = tagName.substring(1); // Remove 'v'
    }
    
    if (tagName.isEmpty()) {
        Serial.println("[OTA] No tag name found in release.");
        return false;
    }
    
    // Simple string comparison works well enough. Any different tag that is latest means a new release
    if (tagName != String(FIRMWARE_VERSION) && tagName != "1.0.0-dev") {
        // Find firmware.bin in assets
        JsonArray assets = doc["assets"];
        bool assetFound = false;
        for (JsonObject asset : assets) {
            String assetName = asset["name"] | "";
            if (assetName == "firmware.bin") {
                _downloadUrl = asset["browser_download_url"] | "";
                _latestVersion = tagName;
                _updateAvailable = true;
                assetFound = true;
                Serial.printf("[OTA] Update available! %s -> %s\n", FIRMWARE_VERSION, _latestVersion.c_str());
                return true;
            }
        }
        if (!assetFound) {
            Serial.println("[OTA] Release found but firmware.bin not attached.");
        }
    } else {
        Serial.println("[OTA] Already up to date.");
        _updateAvailable = false;
    }
    
    return false;
}

bool OtaUpdater::startUpdate() {
    if (!_updateAvailable || _downloadUrl.isEmpty()) {
        return false;
    }
    
    Serial.printf("[OTA] Starting update from %s\n", _downloadUrl.c_str());
    
    WiFiClientSecure* client = new WiFiClientSecure();
    if (!client) return false;
    
    client->setInsecure();
    
    // HTTPUpdate natively supports handling redirects (GitHub returns 302 to AWS S3)
    httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    
    t_httpUpdate_return ret = httpUpdate.update(*client, _downloadUrl);
    
    delete client;
    
    switch (ret) {
        case HTTP_UPDATE_FAILED:
            Serial.printf("[OTA] Update failed. Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
            return false;
        case HTTP_UPDATE_NO_UPDATES:
            Serial.println("[OTA] No updates");
            return false;
        case HTTP_UPDATE_OK:
            Serial.println("[OTA] Update OK! Restarting...");
            // ESP will auto-restart
            return true;
    }
    
    return false;
}
