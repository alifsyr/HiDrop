
#pragma once

#include <stdint.h>

#ifndef APP_FIREBASE_HOST
#define APP_FIREBASE_HOST ""
#endif

#ifndef APP_FIREBASE_AUTH
#define APP_FIREBASE_AUTH ""
#endif

#ifndef APP_GOOGLE_SHEETS_WEB_APP_URL
#define APP_GOOGLE_SHEETS_WEB_APP_URL ""
#endif

#ifndef APP_GOOGLE_SHEETS_SHARED_SECRET
#define APP_GOOGLE_SHEETS_SHARED_SECRET ""
#endif

#ifndef APP_GOOGLE_SHEETS_LOGGING_ENABLED
#define APP_GOOGLE_SHEETS_LOGGING_ENABLED false
#endif

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "1.0.0-dev"
#endif

namespace AppConfig {
    constexpr float ADC_VREF = 3.3f;
    constexpr float ADC_RANGE = 4095.0f;

    constexpr float DEFAULT_WATER_TEMPERATURE_C = 25.0f;

    constexpr int PH_SAMPLE_COUNT = 10;

    constexpr float TDS_SMOOTHING_ALPHA = 0.15f;
    constexpr float PH_SLOPE = -5.70f;
    constexpr float PH_CALIBRATION_VALUE = 21.34f - 0.7f;

    // DS18B20
    constexpr unsigned long TEMP_REQUEST_INTERVAL_MS = 1000;
    constexpr unsigned long TEMP_CONVERSION_DELAY_MS = 200;

    // Sensor scheduling (avoid pH/TDS interference)
    constexpr unsigned long TDS_UPDATE_INTERVAL_MS = 1000;
    constexpr unsigned long PH_UPDATE_INTERVAL_MS = 1500;
    constexpr unsigned long PH_QUIET_AFTER_TDS_MS = 500;

    // Auto dosing target defaults (adult lettuce)
    constexpr float DEFAULT_PH_TARGET_MIN = 5.8f;
    constexpr float DEFAULT_PH_TARGET_MAX = 6.2f;
    constexpr float DEFAULT_PPM_TARGET_MIN = 600.0f;
    constexpr float DEFAULT_PPM_TARGET_MAX = 800.0f;

    // Relay behavior
    constexpr bool RELAY_ACTIVE_LOW = true;

    // Watchdog Timer
    constexpr uint32_t WDT_TIMEOUT_MS = 30000;

    // Pump calibration defaults
    // Calibrate each pump on the real system before production use.
    constexpr float NUTRI_A_FLOW_ML_PER_SEC = 0.25f;
    constexpr float NUTRI_B_FLOW_ML_PER_SEC = 0.25f;
    constexpr float PH_DOWN_FLOW_ML_PER_SEC = 0.25f;
    constexpr float PH_UP_FLOW_ML_PER_SEC = 0.25f;

    // Safe dosing step defaults
    constexpr float NUTRI_A_DOSE_STEP_ML = 2.0f;
    constexpr float NUTRI_B_DOSE_STEP_ML = 2.0f;
    constexpr float PH_DOWN_DOSE_STEP_ML = 0.5f;
    constexpr float PH_UP_DOSE_STEP_ML = 0.5f;

    constexpr uint8_t MAX_NUTRIENT_DOSE_CYCLES = 4;
    constexpr uint8_t MAX_PH_DOSE_CYCLES = 4;
    constexpr unsigned long AUTODOSE_STARTUP_DELAY_MS = 60000;
    constexpr unsigned long AUTODOSE_INTER_PUMP_DELAY_MS = 2000;
    constexpr unsigned long AUTODOSE_RECHECK_DELAY_MS = 10UL * 60UL * 1000UL;
    constexpr unsigned long AUTODOSE_EVENT_COOLDOWN_MS = 15UL * 60UL * 1000UL;

    // LCD 20x4 (I2C)
    constexpr uint8_t LCD_I2C_ADDRESS = 0x27;
    constexpr uint8_t LCD_COLUMNS = 20;
    constexpr uint8_t LCD_ROWS = 4;
    constexpr unsigned long LCD_REFRESH_INTERVAL_MS = 300;
    constexpr unsigned long LCD_SCROLL_INTERVAL_MS = 300;
    constexpr unsigned long LCD_INIT_FINISH_DURATION_MS = 2000;
    constexpr unsigned long LCD_TARGET_MESSAGE_DURATION_MS = 3000;
    constexpr uint8_t LCD_SCROLL_GAP_CHARS = 3;

    // NTP (WIB / UTC+7)
    constexpr char NTP_SERVER_PRIMARY[] = "pool.ntp.org";
    constexpr char NTP_SERVER_SECONDARY[] = "time.nist.gov";
    constexpr long WIB_UTC_OFFSET_SECONDS = 7L * 60L * 60L;

    // Firebase Realtime Database (credentials loaded from .env via load_env.py)
    constexpr bool FIREBASE_ENABLED = true;
    constexpr char FIREBASE_HOST[] = APP_FIREBASE_HOST;
    constexpr char FIREBASE_AUTH[] = APP_FIREBASE_AUTH;
    constexpr unsigned long FIREBASE_UPDATE_INTERVAL_MS = 5000;

    // History sampling interval (used by firebase_client to sample sensor history)
    constexpr unsigned long WEB_DASHBOARD_HISTORY_SAMPLE_INTERVAL_MS = 15000;

    // Google Sheets logging
    constexpr bool GOOGLE_SHEETS_LOGGING_ENABLED = APP_GOOGLE_SHEETS_LOGGING_ENABLED;
    constexpr char GOOGLE_SHEETS_WEB_APP_URL[] = APP_GOOGLE_SHEETS_WEB_APP_URL;
    constexpr char GOOGLE_SHEETS_SHARED_SECRET[] = APP_GOOGLE_SHEETS_SHARED_SECRET;
    constexpr unsigned long GOOGLE_SHEETS_RETRY_INTERVAL_MS = 60000;
    constexpr uint16_t GOOGLE_SHEETS_TIMEOUT_MS = 10000;
}
