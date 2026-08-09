// ============================================================================
//  results_store.h — cache the most recent scan to LittleFS
// ----------------------------------------------------------------------------
//  RAM is lost across deep sleep, so we persist the last scan (plus device
//  GeoIP) to /last_scan.json. A button (power) wake can then show results
//  instantly without re-scanning; a timer wake re-scans and overwrites.
// ============================================================================
#pragma once

#include <Arduino.h>
#include <vector>
#include "config.h"
#include "wifi_scanner.h"

class ResultsStore {
 public:
  bool save(const std::vector<ScannedAP>& nets, const String& ip,
            const String& country, const String& city);

  bool load(std::vector<ScannedAP>& nets, String& ip, String& country,
            String& city);

  bool exists();
};
