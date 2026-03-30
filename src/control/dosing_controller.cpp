#include "control/dosing_controller.h"

#include "config/app_config.h"
#include "config/pins.h"

namespace {
constexpr unsigned long kMinRelayOnMs = 500;
}

DosingController::DosingController()
    : _state(State::IDLE),
      _pendingAction(Action::NONE),
      _eventActive(false),
      _hasCompletedReport(false),
      _startupReadyMs(0),
      _cooldownUntilMs(0),
      _stateStartedMs(0),
      _nutrientCycles(0),
      _phCycles(0) {}

void DosingController::begin() {
    pinMode(Pins::RELAY_NUTRI_A, OUTPUT);
    pinMode(Pins::RELAY_NUTRI_B, OUTPUT);
    pinMode(Pins::RELAY_PH_DOWN, OUTPUT);
    pinMode(Pins::RELAY_PH_UP, OUTPUT);

    stopAllRelays();
    _startupReadyMs = millis() + AppConfig::AUTODOSE_STARTUP_DELAY_MS;
}

void DosingController::update(
    const SensorData &data,
    bool calibrationMode,
    const struct tm *localTime,
    bool timeValid,
    const TargetRanges &targetRanges
) {
    const unsigned long now = millis();

    if (calibrationMode) {
        stopAllRelays();
        _state = State::IDLE;
        _eventActive = false;
        return;
    }

    switch (_state) {
        case State::IDLE: {
            if (now < _startupReadyMs || now < _cooldownUntilMs) {
                return;
            }

            const Action action = chooseAction(data, targetRanges);
            if (action == Action::NONE) {
                return;
            }

            startEvent(data, localTime, timeValid);
            _pendingAction = action;

            if (action == Action::MANUAL_DILUTION_REQUIRED) {
                _activeReport.manualDilutionRequired = true;
                finalizeEvent(data, "Manual dilution required because PPM is above target.");
                return;
            }

            if (action == Action::DOSE_NUTRIENTS) {
                _nutrientCycles++;
                startRelayState(
                    State::DOSING_NUTRI_A,
                    Pins::RELAY_NUTRI_A,
                    AppConfig::NUTRI_A_DOSE_STEP_ML,
                    "Dosing Nutrient A"
                );
                return;
            }

            if (action == Action::DOSE_PH_DOWN) {
                _phCycles++;
                startRelayState(
                    State::DOSING_PH_DOWN,
                    Pins::RELAY_PH_DOWN,
                    AppConfig::PH_DOWN_DOSE_STEP_ML,
                    "Dosing pH Down"
                );
                return;
            }

            if (action == Action::DOSE_PH_UP) {
                _phCycles++;
                startRelayState(
                    State::DOSING_PH_UP,
                    Pins::RELAY_PH_UP,
                    AppConfig::PH_UP_DOSE_STEP_ML,
                    "Dosing pH Up"
                );
            }
            return;
        }

        case State::DOSING_NUTRI_A:
            if (now - _stateStartedMs >= doseDurationMs(AppConfig::NUTRI_A_DOSE_STEP_ML, AppConfig::NUTRI_A_FLOW_ML_PER_SEC)) {
                setRelay(Pins::RELAY_NUTRI_A, false);
                _state = State::PAUSE_BEFORE_NUTRI_B;
                _stateStartedMs = now;
                Serial.println("Nutrient A dose complete.");
            }
            return;

        case State::PAUSE_BEFORE_NUTRI_B:
            if (now - _stateStartedMs >= AppConfig::AUTODOSE_INTER_PUMP_DELAY_MS) {
                startRelayState(
                    State::DOSING_NUTRI_B,
                    Pins::RELAY_NUTRI_B,
                    AppConfig::NUTRI_B_DOSE_STEP_ML,
                    "Dosing Nutrient B"
                );
            }
            return;

        case State::DOSING_NUTRI_B:
            if (now - _stateStartedMs >= doseDurationMs(AppConfig::NUTRI_B_DOSE_STEP_ML, AppConfig::NUTRI_B_FLOW_ML_PER_SEC)) {
                setRelay(Pins::RELAY_NUTRI_B, false);
                _state = State::WAITING_RECHECK;
                _stateStartedMs = now;
                Serial.println("Nutrient B dose complete. Waiting for solution mixing.");
            }
            return;

        case State::DOSING_PH_DOWN:
            if (now - _stateStartedMs >= doseDurationMs(AppConfig::PH_DOWN_DOSE_STEP_ML, AppConfig::PH_DOWN_FLOW_ML_PER_SEC)) {
                setRelay(Pins::RELAY_PH_DOWN, false);
                _state = State::WAITING_RECHECK;
                _stateStartedMs = now;
                Serial.println("pH Down dose complete. Waiting for solution mixing.");
            }
            return;

        case State::DOSING_PH_UP:
            if (now - _stateStartedMs >= doseDurationMs(AppConfig::PH_UP_DOSE_STEP_ML, AppConfig::PH_UP_FLOW_ML_PER_SEC)) {
                setRelay(Pins::RELAY_PH_UP, false);
                _state = State::WAITING_RECHECK;
                _stateStartedMs = now;
                Serial.println("pH Up dose complete. Waiting for solution mixing.");
            }
            return;

        case State::WAITING_RECHECK:
            if (now - _stateStartedMs < AppConfig::AUTODOSE_RECHECK_DELAY_MS) {
                return;
            }

            _pendingAction = chooseAction(data, targetRanges);
            if (_pendingAction == Action::NONE) {
                finalizeEvent(data, "Targets reached.");
                return;
            }

            if (_pendingAction == Action::MANUAL_DILUTION_REQUIRED) {
                _activeReport.manualDilutionRequired = true;
                finalizeEvent(data, "Manual dilution required after dosing cycle.");
                return;
            }

            if (_pendingAction == Action::DOSE_NUTRIENTS) {
                if (_nutrientCycles >= AppConfig::MAX_NUTRIENT_DOSE_CYCLES) {
                    finalizeEvent(data, "Maximum nutrient dosing cycles reached.");
                    return;
                }

                _nutrientCycles++;
                startRelayState(
                    State::DOSING_NUTRI_A,
                    Pins::RELAY_NUTRI_A,
                    AppConfig::NUTRI_A_DOSE_STEP_ML,
                    "Continuing nutrient correction with Nutrient A"
                );
                return;
            }

            if (_pendingAction == Action::DOSE_PH_DOWN) {
                if (_phCycles >= AppConfig::MAX_PH_DOSE_CYCLES) {
                    finalizeEvent(data, "Maximum pH down dosing cycles reached.");
                    return;
                }

                _phCycles++;
                startRelayState(
                    State::DOSING_PH_DOWN,
                    Pins::RELAY_PH_DOWN,
                    AppConfig::PH_DOWN_DOSE_STEP_ML,
                    "Continuing pH correction with pH Down"
                );
                return;
            }

            if (_pendingAction == Action::DOSE_PH_UP) {
                if (_phCycles >= AppConfig::MAX_PH_DOSE_CYCLES) {
                    finalizeEvent(data, "Maximum pH up dosing cycles reached.");
                    return;
                }

                _phCycles++;
                startRelayState(
                    State::DOSING_PH_UP,
                    Pins::RELAY_PH_UP,
                    AppConfig::PH_UP_DOSE_STEP_ML,
                    "Continuing pH correction with pH Up"
                );
            }
            return;
    }
}

bool DosingController::isBusy() const {
    return _state != State::IDLE;
}

bool DosingController::consumeCompletedReport(DosingReport &report) {
    if (!_hasCompletedReport) {
        return false;
    }

    report = _completedReport;
    _hasCompletedReport = false;
    return true;
}

DosingController::Action DosingController::chooseAction(const SensorData &data, const TargetRanges &targetRanges) const {
    if (data.tds > targetRanges.ppmMax) {
        return Action::MANUAL_DILUTION_REQUIRED;
    }

    if (data.tds < targetRanges.ppmMin) {
        return Action::DOSE_NUTRIENTS;
    }

    if (data.phValue > targetRanges.phMax) {
        return Action::DOSE_PH_DOWN;
    }

    if (data.phValue < targetRanges.phMin) {
        return Action::DOSE_PH_UP;
    }

    return Action::NONE;
}

unsigned long DosingController::doseDurationMs(float ml, float flowMlPerSecond) const {
    if (flowMlPerSecond <= 0.0f) {
        return kMinRelayOnMs;
    }

    const unsigned long duration = static_cast<unsigned long>((ml / flowMlPerSecond) * 1000.0f);
    return duration < kMinRelayOnMs ? kMinRelayOnMs : duration;
}

void DosingController::startEvent(const SensorData &data, const struct tm *localTime, bool timeValid) {
    _activeReport = DosingReport();
    _activeReport.temperatureC = data.temperatureC;
    _activeReport.ppmStart = data.tds;
    _activeReport.ppmEnd = data.tds;
    _activeReport.phStart = data.phValue;
    _activeReport.phEnd = data.phValue;
    _nutrientCycles = 0;
    _phCycles = 0;
    _eventActive = true;

    if (timeValid && localTime != nullptr) {
        _activeReport.date = formatDate(*localTime);
        _activeReport.time = formatTime(*localTime);
    }

    Serial.println("Auto dosing event started.");
}

void DosingController::finalizeEvent(const SensorData &data, const char *reason) {
    stopAllRelays();
    _activeReport.temperatureC = data.temperatureC;
    _activeReport.ppmEnd = data.tds;
    _activeReport.phEnd = data.phValue;
    _completedReport = _activeReport;
    _hasCompletedReport = true;
    _eventActive = false;
    _state = State::IDLE;
    _pendingAction = Action::NONE;
    _cooldownUntilMs = millis() + AppConfig::AUTODOSE_EVENT_COOLDOWN_MS;

    Serial.print("Auto dosing event completed: ");
    Serial.println(reason);
}

void DosingController::startRelayState(State nextState, uint8_t pin, float addedMl, const char *label) {
    setRelay(pin, true);
    _state = nextState;
    _stateStartedMs = millis();

    if (pin == Pins::RELAY_NUTRI_A) {
        _activeReport.nutrientAMl += addedMl;
    } else if (pin == Pins::RELAY_NUTRI_B) {
        _activeReport.nutrientBMl += addedMl;
    } else if (pin == Pins::RELAY_PH_DOWN) {
        _activeReport.phDownMl += addedMl;
    } else if (pin == Pins::RELAY_PH_UP) {
        _activeReport.phUpMl += addedMl;
    }

    Serial.println(label);
}

void DosingController::stopAllRelays() {
    setRelay(Pins::RELAY_NUTRI_A, false);
    setRelay(Pins::RELAY_NUTRI_B, false);
    setRelay(Pins::RELAY_PH_DOWN, false);
    setRelay(Pins::RELAY_PH_UP, false);
}

void DosingController::setRelay(uint8_t pin, bool active) {
    const bool activeLow = AppConfig::RELAY_ACTIVE_LOW;
    digitalWrite(pin, active ? (activeLow ? LOW : HIGH) : (activeLow ? HIGH : LOW));
}

String DosingController::formatDate(const struct tm &localTime) const {
    char buffer[17];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", &localTime);
    return String(buffer);
}

String DosingController::formatTime(const struct tm &localTime) const {
    char buffer[9];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", &localTime);
    return String(buffer);
}
