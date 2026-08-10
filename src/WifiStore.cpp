#include "WifiStore.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

#include "config.h"

namespace WifiStore {

bool load(String& ssid, String& pass) {
  File f = LittleFS.open(WIFI_PATH, "r");
  if (!f) return false;
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return false;
  ssid = doc["ssid"] | "";
  pass = doc["pass"] | "";
  return ssid.length() > 0;
}

bool has() {
  String s, p;
  return load(s, p);
}

void save(const String& ssid, const String& pass) {
  JsonDocument doc;
  doc["ssid"] = ssid;
  doc["pass"] = pass;
  File w = LittleFS.open(WIFI_PATH, "w");
  if (!w) return;
  serializeJson(doc, w);
  w.close();
}

}  // namespace WifiStore
