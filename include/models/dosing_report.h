#pragma once

#include <Arduino.h>

struct DosingReport {
    String date = "N/A";
    String time = "N/A";
    float temperatureC = 0.0f;
    float ppmStart = 0.0f;
    float ppmEnd = 0.0f;
    float nutrientAMl = 0.0f;
    float nutrientBMl = 0.0f;
    float phStart = 0.0f;
    float phEnd = 0.0f;
    float phDownMl = 0.0f;
    float phUpMl = 0.0f;
    bool manualDilutionRequired = false;
};
