// ============================================================================
//  osint_db.h — offline enrichment: MAC/OUI vendor lookup + known-network check
// ----------------------------------------------------------------------------
//  Ships with a small built-in OUI table covering common consumer vendors.
//  If /oui.csv (lines: "AABBCC,Vendor Name") is present on LittleFS it is used
//  as an extended fallback. Known networks are read from /known_networks.txt
//  (one SSID or BSSID per line) plus the configured home SSID.
// ============================================================================
#pragma once

#include <Arduino.h>
#include <vector>
#include "config.h"

class OsintDB {
 public:
  // Loads optional known-networks list and notes whether an extended OUI CSV
  // is available on the filesystem. Safe to call even if files are absent.
  bool begin();

  // The home SSID is always treated as "known".
  void setHomeSsid(const String& ssid) { homeSsid_ = ssid; }

  // Vendor for a 6-byte BSSID. Returns "Unknown" when not found.
  String lookupMAC(const uint8_t bssid[6]);

  // Vendor for a "AA:BB:CC:DD:EE:FF" (or "AABBCC...") string.
  String lookupMAC(const String& macStr);

  // True if the SSID or BSSID matches the home network or the known list.
  bool isKnown(const String& ssid, const uint8_t bssid[6]);

 private:
  String  homeSsid_;
  bool    hasCsv_ = false;
  std::vector<String> knownList_;  // SSIDs and/or BSSID strings (upper-case)

  String lookupCsv_(uint32_t oui);
  static uint32_t ouiFromBytes_(const uint8_t bssid[6]);
  static String bssidToString_(const uint8_t bssid[6]);
};
