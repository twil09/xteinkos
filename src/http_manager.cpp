#include "http_manager.h"

#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

bool HTTPManager::fetchString(const char* url, String& response) {
  lastStatus_ = 0;
  lastError_ = "";

  if (WiFi.status() != WL_CONNECTED) {
    lastError_ = "wifi not connected";
    return false;
  }
  if (!url || strlen(url) == 0) {
    lastError_ = "empty url";
    return false;
  }

  const bool secure = strncmp(url, "https://", 8) == 0;

  // The secure client is heap-allocated only when needed to keep RAM low.
  WiFiClient*        plain  = nullptr;
  WiFiClientSecure*  tls    = nullptr;
  HTTPClient http;
  bool begun = false;

  if (secure) {
    tls = new WiFiClientSecure();
    tls->setInsecure();  // no CA store on device; enrichment data is low-trust
    tls->setTimeout(readTimeoutMs_ / 1000);
    begun = http.begin(*tls, url);
  } else {
    plain = new WiFiClient();
    plain->setTimeout(readTimeoutMs_ / 1000);
    begun = http.begin(*plain, url);
  }

  if (!begun) {
    lastError_ = "http.begin failed";
    delete plain;
    delete tls;
    return false;
  }

  http.setConnectTimeout(connectTimeoutMs_);
  http.setTimeout(readTimeoutMs_);
  http.useHTTP10(true);           // stream-friendly for ArduinoJson
  http.addHeader("Accept", "application/json");
  http.setUserAgent("X3-OSINT/" FW_VERSION);

  int code = http.GET();
  lastStatus_ = code;

  bool ok = false;
  if (code > 0 && code >= 200 && code < 300) {
    response = http.getString();
    ok = true;
  } else {
    lastError_ = String("HTTP ") + code;
    if (code < 0) lastError_ = HTTPClient::errorToString(code);
  }

  http.end();
  delete plain;
  delete tls;
  return ok;
}

bool HTTPManager::fetchJSON(const char* url, JsonDocument& doc) {
  String body;
  if (!fetchString(url, body)) return false;

  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    lastError_ = String("json: ") + err.c_str();
    return false;
  }
  return true;
}

bool HTTPManager::fetchGeoIP(const String& serverBase, String& country,
                             String& city) {
  if (serverBase.length() == 0) {
    lastError_ = "no server url";
    return false;
  }
  String base = serverBase;
  while (base.endsWith("/")) base.remove(base.length() - 1);
  String url = base + "/api/geoip";

  JsonDocument doc;
  if (!fetchJSON(url.c_str(), doc)) return false;

  // Accept a few common field spellings.
  const char* c = doc["country"] | (doc["country_name"] | (const char*)nullptr);
  const char* t = doc["city"]    | (const char*)nullptr;
  if (c) country = c;
  if (t) city = t;
  return true;
}
