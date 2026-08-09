// ============================================================================
//  http_manager.h — HTTP(S) fetch + JSON parsing with graceful fallback
// ============================================================================
#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "config.h"

class HTTPManager {
 public:
  // Fetch `url` and parse the body as JSON into `doc`.
  // Returns false on transport error, non-2xx status, or parse failure.
  bool fetchJSON(const char* url, JsonDocument& doc);

  // Fetch `url` and return the raw body in `response`.
  bool fetchString(const char* url, String& response);

  void setConnectTimeout(int ms) { connectTimeoutMs_ = ms; }
  void setReadTimeout(int ms)    { readTimeoutMs_ = ms; }

  // Convenience: GET <base>/api/geoip and pull country/city from the JSON.
  // The server resolves the caller's public IP. Returns false on any failure;
  // country/city are left untouched so callers can keep cached values.
  bool fetchGeoIP(const String& serverBase, String& country, String& city);

  // Last HTTP status / error text for diagnostics.
  int          lastStatus() const { return lastStatus_; }
  const String& lastError() const { return lastError_; }

 private:
  int    connectTimeoutMs_ = HTTP_CONNECT_TIMEOUT_MS;
  int    readTimeoutMs_    = HTTP_READ_TIMEOUT_MS;
  int    lastStatus_       = 0;
  String lastError_;
};
