#pragma once

#include <Arduino.h>
#include <time.h>

#include "models/dosing_report.h"
#include "models/sensor_data.h"

class DosingController {
public:
    DosingController();

    void begin();
    void update(
        const SensorData &data,
        bool calibrationMode,
        const struct tm *localTime,
        bool timeValid
    );

    bool isBusy() const;
    bool consumeCompletedReport(DosingReport &report);

private:
    enum class Action {
        NONE,
        DOSE_NUTRIENTS,
        DOSE_PH_DOWN,
        DOSE_PH_UP,
        MANUAL_DILUTION_REQUIRED
    };

    enum class State {
        IDLE,
        DOSING_NUTRI_A,
        PAUSE_BEFORE_NUTRI_B,
        DOSING_NUTRI_B,
        DOSING_PH_DOWN,
        DOSING_PH_UP,
        WAITING_RECHECK
    };

    State _state;
    Action _pendingAction;
    DosingReport _activeReport;
    DosingReport _completedReport;
    bool _eventActive;
    bool _hasCompletedReport;

    unsigned long _startupReadyMs;
    unsigned long _cooldownUntilMs;
    unsigned long _stateStartedMs;
    uint8_t _nutrientCycles;
    uint8_t _phCycles;

    Action chooseAction(const SensorData &data) const;
    unsigned long doseDurationMs(float ml, float flowMlPerSecond) const;
    void startEvent(const SensorData &data, const struct tm *localTime, bool timeValid);
    void finalizeEvent(const SensorData &data, const char *reason);
    void startRelayState(State nextState, uint8_t pin, float addedMl, const char *label);
    void stopAllRelays();
    void setRelay(uint8_t pin, bool active);
    String formatDate(const struct tm &localTime) const;
    String formatTime(const struct tm &localTime) const;
};
