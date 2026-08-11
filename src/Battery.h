// Battery — read the X3 LiPo gauge (ADC on GPIO0, 2.0x divider).
// Math ported from the MIT community-sdk BatteryMonitor (LiPo polynomial).
#pragma once
#include <Arduino.h>
#include <algorithm>
#include <cmath>
#include "config.h"

namespace Battery {

// Averaged raw battery voltage in millivolts (after the divider).
inline uint16_t millivolts() {
  long sum = 0;
  for (int i = 0; i < 4; ++i) sum += analogReadMilliVolts(BATTERY_ADC_PIN);
  return (uint16_t)((sum / 4) * BATTERY_DIVIDER);
}

// 0-100% from the LiPo discharge curve, clamped.
inline uint16_t percent() {
  double v = millivolts() / 1000.0;
  double y = -144.9390 * v * v * v + 1655.8629 * v * v - 6158.8520 * v + 7501.3202;
  y = std::max(0.0, std::min(100.0, y));
  return (uint16_t)lround(y);
}

}  // namespace Battery
