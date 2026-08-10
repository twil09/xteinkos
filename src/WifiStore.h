// WifiStore — persist the home WiFi credentials to LittleFS (/wifi.json).
#pragma once
#include <Arduino.h>

namespace WifiStore {
bool has();
bool load(String& ssid, String& pass);
void save(const String& ssid, const String& pass);
}  // namespace WifiStore
