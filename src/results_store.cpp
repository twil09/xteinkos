#include "results_store.h"

#include <LittleFS.h>
#include <ArduinoJson.h>

static void parseBssid(const String& s, uint8_t out[6]) {
  for (int i = 0; i < 6; ++i) out[i] = 0;
  int idx = 0;
  unsigned cur = 0, nib = 0;
  for (size_t i = 0; i < s.length() && idx < 6; ++i) {
    char c = s[i];
    int v;
    if (c >= '0' && c <= '9') v = c - '0';
    else if (c >= 'a' && c <= 'f') v = c - 'a' + 10;
    else if (c >= 'A' && c <= 'F') v = c - 'A' + 10;
    else continue;
    cur = (cur << 4) | v;
    if (++nib == 2) { out[idx++] = (uint8_t)cur; cur = 0; nib = 0; }
  }
}

bool ResultsStore::exists() { return LittleFS.exists(LAST_SCAN_PATH); }

bool ResultsStore::save(const std::vector<ScannedAP>& nets, const String& ip,
                        const String& country, const String& city) {
  JsonDocument doc;
  doc["ip"] = ip;
  doc["country"] = country;
  doc["city"] = city;
  doc["uptime"] = (uint32_t)millis();
  JsonArray arr = doc["nets"].to<JsonArray>();
  for (const auto& ap : nets) {
    JsonObject o = arr.add<JsonObject>();
    o["ssid"] = ap.ssid;
    o["bssid"] = ap.bssidStr;
    o["rssi"] = ap.rssi;
    o["ch"] = ap.channel;
    o["auth"] = ap.encryption;
    o["vendor"] = ap.vendor;
    o["known"] = ap.isKnown;
  }

  File f = LittleFS.open(LAST_SCAN_PATH, "w");
  if (!f) return false;
  size_t n = serializeJson(doc, f);
  f.close();
  return n > 0;
}

bool ResultsStore::load(std::vector<ScannedAP>& nets, String& ip,
                        String& country, String& city) {
  nets.clear();
  File f = LittleFS.open(LAST_SCAN_PATH, "r");
  if (!f) return false;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return false;

  ip = doc["ip"] | "";
  country = doc["country"] | "";
  city = doc["city"] | "";

  for (JsonObject o : doc["nets"].as<JsonArray>()) {
    ScannedAP ap;
    ap.ssid = o["ssid"] | "";
    ap.bssidStr = o["bssid"] | "";
    parseBssid(ap.bssidStr, ap.bssid);
    ap.rssi = o["rssi"] | 0;
    ap.channel = o["ch"] | 0;
    ap.encryption = o["auth"] | 0;
    ap.vendor = o["vendor"] | "Unknown";
    ap.isKnown = o["known"] | false;
    nets.push_back(ap);
  }
  return true;
}
