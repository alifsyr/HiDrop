#pragma once

#include <Arduino.h>
#include <GravityTDS.h>
#include "models/sensor_data.h"

class TdsSensor {
  public:
      TdsSensor(int pin, float aref, float adcRange, float smoothingAlpha = 0.15f);
  
      void begin();
      void update(float temperatureC);
      void calibrate(const char* cmd);
  
      float getTds() const;
      float getKvalue();
      SensorData getData(float temperatureC) const;
  
  private:
      int _pin;
      float _aref;
      float _adcRange;
      float _lastTds;
      float _smoothingAlpha;
      bool _initialized;
  
      GravityTDS _gravityTds;
};