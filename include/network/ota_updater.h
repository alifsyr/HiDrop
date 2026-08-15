#pragma once

#include <Arduino.h>

class OtaUpdater {
public:
    OtaUpdater(const String& repoOwner, const String& repoName);
    
    void begin();
    void update(bool forceCheck = false);
    
    bool isUpdateAvailable() const { return _updateAvailable; }
    String getLatestVersion() const { return _latestVersion; }
    String getCurrentVersion() const { return FIRMWARE_VERSION; }
    
    // Call this when user wants to start the OTA update process
    bool startUpdate();

private:
    String _repoOwner;
    String _repoName;
    bool _updateAvailable;
    String _latestVersion;
    String _downloadUrl;
    unsigned long _lastCheckMs;
    
    bool checkForUpdate();
};
