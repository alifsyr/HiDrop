#pragma once

namespace AppConfig {
    constexpr float ADC_VREF = 3.3f;
    constexpr float ADC_RANGE = 4095.0f;

    constexpr float DEFAULT_WATER_TEMPERATURE_C = 25.0f;
    constexpr unsigned long TDS_PUBLISH_INTERVAL_MS = 1000;

    constexpr int PH_SAMPLE_COUNT = 10;
    constexpr unsigned long SENSOR_PRINT_INTERVAL_MS = 1000;

    // Filter smoothing di TDS: 0.0..1.0 (kecil = lebih stabil, lebih lambat responsif)
    constexpr float TDS_SMOOTHING_ALPHA = 0.15f;

    // Sementara pakai referensi dari kode Anda
    constexpr float PH_SLOPE = -5.70f;
    constexpr float PH_CALIBRATION_VALUE = 21.34f - 0.7f;

    // DS18B20
    constexpr unsigned long TEMP_REQUEST_INTERVAL_MS = 1000;
    constexpr unsigned long TEMP_CONVERSION_DELAY_MS = 200;
}