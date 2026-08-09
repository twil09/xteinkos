// ============================================================================
//  wifi_scanner.h — passive WiFi environment scan + enrichment
// ----------------------------------------------------------------------------
//  Wraps WiFi.scanNetworks() (passive listening to broadcast beacons — the
//  same data any phone's WiFi list shows) and decorates each result with
//  offline vendor/known-network info from OsintDB.
// ============================================================================
#pragma once

#include <Arduino.h>
#include <vector>
#include "config.h"
#include "osint_db.h"

struct ScannedAP {
  String   ssid;
  uint8_t  bssid[6] = {0};
  String   bssidStr;
  int32_t  rssi     = 0;
  uint8_t  channel  = 0;
  uint8_t  encryption = 0;   // wifi_auth_mode_t
  String   vendor   = "Unknown";
  bool     isKnown  = false;
  // Optional HTTP-enriched fields (device-level GeoIP, see HTTPManager).
  String   country;
  String   city;
};

class WiFiScanner {
 public:
  // Raw scan. Fills `out` (cleared first), returns AP count (0..MAX_SCAN_RESULTS).
  int scan(std::vector<ScannedAP>& out);

  // Adds vendor + isKnown to each result using the offline DB.
  void enrich(std::vector<ScannedAP>& out, OsintDB& db);

  // Convenience: scan() followed by enrich().
  int scanEnriched(std::vector<ScannedAP>& out, OsintDB& db);

  // Human-readable auth mode.
  static const char* authModeStr(uint8_t mode);
};
