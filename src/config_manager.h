// ============================================================================
//  config_manager.h — persistent device configuration (LittleFS + JSON)
// ============================================================================
#pragma once

#include <Arduino.h>
#include "config.h"

struct DeviceConfig {
  String    ssid;                                   // home WiFi to join
  String    password;                               // home WiFi password
  OsintMode mode        = OsintMode::OsintMode;     // reconnaissance mode
  String    serverUrl;                              // optional data server base URL
  uint32_t  scanInterval = DEFAULT_SCAN_INTERVAL_S; // deep-sleep seconds
  bool      configured  = false;                    // false => run captive portal
};

class ConfigManager {
 public:
  // Mounts LittleFS (formatting on first use). Returns false if the FS could
  // not be mounted at all.
  bool begin();

  // True if a valid, "configured": true config file is present.
  bool exists();

  // Load config from CONFIG_PATH into `out`. Returns false on missing/invalid
  // file (out is left at defaults).
  bool load(DeviceConfig& out);

  // Persist `cfg` (sets configured=true) to CONFIG_PATH. Returns false on I/O
  // or serialization error.
  bool save(const DeviceConfig& cfg);

  // Remove the config file, forcing the captive portal on next boot.
  bool clear();

 private:
  bool mounted_ = false;
};
