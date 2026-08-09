#include "config_manager.h"

#include <LittleFS.h>
#include <ArduinoJson.h>

bool ConfigManager::begin() {
  // format-on-fail = true so a virgin device comes up with a usable FS.
  mounted_ = LittleFS.begin(true);
  if (!mounted_) {
    Serial.println(F("[config] LittleFS mount FAILED"));
  }
  return mounted_;
}

bool ConfigManager::exists() {
  if (!mounted_) return false;
  if (!LittleFS.exists(CONFIG_PATH)) return false;
  DeviceConfig probe;
  return load(probe) && probe.configured;
}

bool ConfigManager::load(DeviceConfig& out) {
  if (!mounted_) return false;

  File f = LittleFS.open(CONFIG_PATH, "r");
  if (!f) {
    Serial.println(F("[config] no config file"));
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) {
    Serial.printf("[config] parse error: %s\n", err.c_str());
    return false;
  }

  out.ssid         = doc["ssid"] | "";
  out.password     = doc["password"] | "";
  out.serverUrl    = doc["server_url"] | "";
  out.mode         = parseMode(doc["mode"] | "osint");
  out.scanInterval = doc["scan_interval"] | (uint32_t)DEFAULT_SCAN_INTERVAL_S;
  out.configured   = doc["configured"] | false;

  if (out.scanInterval < 30) out.scanInterval = 30;  // sanity floor
  return true;
}

bool ConfigManager::save(const DeviceConfig& cfg) {
  if (!mounted_) return false;

  JsonDocument doc;
  doc["ssid"]          = cfg.ssid;
  doc["password"]      = cfg.password;
  doc["mode"]          = modeToken(cfg.mode);
  doc["server_url"]    = cfg.serverUrl;
  doc["scan_interval"] = cfg.scanInterval;
  doc["configured"]    = true;

  File f = LittleFS.open(CONFIG_PATH, "w");
  if (!f) {
    Serial.println(F("[config] open for write FAILED"));
    return false;
  }
  size_t written = serializeJsonPretty(doc, f);
  f.close();

  if (written == 0) {
    Serial.println(F("[config] write produced 0 bytes"));
    return false;
  }
  Serial.printf("[config] saved %u bytes to %s\n", (unsigned)written, CONFIG_PATH);
  return true;
}

bool ConfigManager::clear() {
  if (!mounted_) return false;
  if (!LittleFS.exists(CONFIG_PATH)) return true;
  return LittleFS.remove(CONFIG_PATH);
}
