#pragma once

#include <stdint.h>

namespace AppConfig {
    constexpr float ADC_VREF = 3.3f;
    constexpr float ADC_RANGE = 4095.0f;

    constexpr float DEFAULT_WATER_TEMPERATURE_C = 25.0f;

    constexpr int PH_SAMPLE_COUNT = 10;
    constexpr unsigned long SENSOR_PRINT_INTERVAL_MS = 1000;

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

    // LCD 20x4 (I2C)
    constexpr uint8_t LCD_I2C_ADDRESS = 0x27;
    constexpr uint8_t LCD_COLUMNS = 20;
    constexpr uint8_t LCD_ROWS = 4;
    constexpr unsigned long LCD_REFRESH_INTERVAL_MS = 300;
    constexpr unsigned long LCD_SCROLL_INTERVAL_MS = 300;
    constexpr unsigned long LCD_INIT_FINISH_DURATION_MS = 2000;
    constexpr uint8_t LCD_SCROLL_GAP_CHARS = 3;

    // Wi-Fi + NTP (WIB / UTC+7)
    constexpr char WIFI_SSID[] = "Tabrin";
    constexpr char WIFI_PASSWORD[] = "qawsed123";
    constexpr char NTP_SERVER_PRIMARY[] = "pool.ntp.org";
    constexpr char NTP_SERVER_SECONDARY[] = "time.nist.gov";
    constexpr long WIB_UTC_OFFSET_SECONDS = 7L * 60L * 60L;
    constexpr unsigned long WIFI_RECONNECT_INTERVAL_MS = 10000;
}
