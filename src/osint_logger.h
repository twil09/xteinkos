// ============================================================================
//  osint_logger.h — append scan results to a CSV log on LittleFS
// ----------------------------------------------------------------------------
//  Simple, size-bounded CSV logger. Rotates (truncates to a ".1" backup) once
//  the active log passes SCAN_LOG_MAX_BYTES so a long-running device never
//  fills the flash partition.
// ============================================================================
#pragma once

#include <Arduino.h>
#include "config.h"
#include "wifi_scanner.h"

class OsintLogger {
 public:
  bool begin();

  // Writes a session marker line (timestamp/uptime + note).
  void logSession(const char* note);

  // Appends one CSV row for a scanned AP.
  void logWifi(const ScannedAP& ap);

 private:
  bool ready_ = false;
  void rotateIfNeeded_();
  void ensureHeader_();
};
