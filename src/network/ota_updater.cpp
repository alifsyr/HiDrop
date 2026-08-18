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

    // Wait 10 seconds after boot before doing the first OTA check
    // to avoid blocking the main loop while LCD is showing "WiFi connecting".
    if (!forceCheck && now < 10000) {
        return;
    }

    if (forceCheck || _lastCheckMs == 0 || (now - _lastCheckMs) > 43200000UL) {
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
    return startUpdateFromUrl(_downloadUrl, nullptr);
}

bool OtaUpdater::startUpdateFromUrl(const String& url, ProgressCallback progressCb) {
    if (url.isEmpty()) return false;
    
    Serial.printf("[OTA] Starting update from URL: %s\n", url.c_str());
    
    WiFiClientSecure* client = new WiFiClientSecure();
    if (!client) return false;
    
    client->setInsecure();
    
    httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    
    // Set progress callback if provided
    int lastReportedPct = -1;
    if (progressCb) {
        httpUpdate.onProgress([progressCb, &lastReportedPct](int current, int total) {
            if (total <= 0) return;
            int pct = (current * 100) / total;
            // Only report on significant changes to avoid flooding Firebase
            if (pct >= lastReportedPct + 5 || pct == 100) {
                lastReportedPct = pct;
                progressCb(pct);
            }
        });
    }
    
    t_httpUpdate_return ret = httpUpdate.update(*client, url);
    
    delete client;
    
    switch (ret) {
        case HTTP_UPDATE_FAILED:
            Serial.printf("[OTA] Update failed. Error (%d): %s\n",
                httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
            return false;
        case HTTP_UPDATE_NO_UPDATES:
            Serial.println("[OTA] No updates available at that URL");
            return false;
        case HTTP_UPDATE_OK:
            Serial.println("[OTA] Update OK! Rebooting...");
            return true; // ESP32 auto-restarts after this
    }
    return false;
}

