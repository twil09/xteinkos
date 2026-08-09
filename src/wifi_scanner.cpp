#include "wifi_scanner.h"

#include <WiFi.h>

int WiFiScanner::scan(std::vector<ScannedAP>& out) {
  out.clear();

  // Ensure the radio is in a state where scanning works even while associated.
  int n = WiFi.scanNetworks(/*async=*/false, /*show_hidden=*/true);
  if (n <= 0) {
    Serial.printf("[scan] %d networks\n", n);
    WiFi.scanDelete();
    return (n < 0) ? 0 : 0;
  }

  int keep = (n > MAX_SCAN_RESULTS) ? MAX_SCAN_RESULTS : n;
  out.reserve(keep);

  for (int i = 0; i < keep; ++i) {
    ScannedAP ap;
    ap.ssid       = WiFi.SSID(i);
    ap.rssi       = WiFi.RSSI(i);
    ap.channel    = WiFi.channel(i);
    ap.encryption = (uint8_t)WiFi.encryptionType(i);

    const uint8_t* b = WiFi.BSSID(i);
    if (b) {
      memcpy(ap.bssid, b, 6);
      char buf[18];
      snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
               b[0], b[1], b[2], b[3], b[4], b[5]);
      ap.bssidStr = buf;
    }
    if (ap.ssid.length() == 0) ap.ssid = "<hidden>";
    out.push_back(ap);
  }

  WiFi.scanDelete();  // free the internal scan buffer
  Serial.printf("[scan] kept %d/%d networks\n", keep, n);
  return keep;
}

void WiFiScanner::enrich(std::vector<ScannedAP>& out, OsintDB& db) {
  for (auto& ap : out) {
    ap.vendor  = db.lookupMAC(ap.bssid);
    ap.isKnown = db.isKnown(ap.ssid, ap.bssid);
  }
}

int WiFiScanner::scanEnriched(std::vector<ScannedAP>& out, OsintDB& db) {
  int n = scan(out);
  enrich(out, db);
  return n;
}

const char* WiFiScanner::authModeStr(uint8_t mode) {
  switch (mode) {
    case WIFI_AUTH_OPEN:            return "Open";
    case WIFI_AUTH_WEP:             return "WEP";
    case WIFI_AUTH_WPA_PSK:         return "WPA";
    case WIFI_AUTH_WPA2_PSK:        return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK:    return "WPA/2";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-Ent";
    case WIFI_AUTH_WPA3_PSK:        return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK:   return "WPA2/3";
    default:                        return "?";
  }
}
