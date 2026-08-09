#include "osint_logger.h"

#include <LittleFS.h>

bool OsintLogger::begin() {
  ready_ = true;   // LittleFS is mounted by ConfigManager during setup()
  ensureHeader_();
  return ready_;
}

void OsintLogger::ensureHeader_() {
  if (LittleFS.exists(SCAN_LOG_PATH)) return;
  File f = LittleFS.open(SCAN_LOG_PATH, "w");
  if (!f) { ready_ = false; return; }
  f.println(F("# type,uptime_ms,ssid,bssid,rssi_dbm,channel,auth,vendor,known,country,city"));
  f.close();
}

void OsintLogger::rotateIfNeeded_() {
  File f = LittleFS.open(SCAN_LOG_PATH, "r");
  if (!f) return;
  size_t sz = f.size();
  f.close();
  if (sz < SCAN_LOG_MAX_BYTES) return;

  // Keep a single backup generation.
  if (LittleFS.exists(SCAN_LOG_PATH ".1")) LittleFS.remove(SCAN_LOG_PATH ".1");
  LittleFS.rename(SCAN_LOG_PATH, SCAN_LOG_PATH ".1");
  ensureHeader_();
  Serial.println(F("[log] rotated scan log"));
}

static String csvEscape(const String& in) {
  // Wrap in quotes and double any embedded quotes if the field needs it.
  bool needsQuote = in.indexOf(',') >= 0 || in.indexOf('"') >= 0 ||
                    in.indexOf('\n') >= 0;
  if (!needsQuote) return in;
  String out = "\"";
  for (size_t i = 0; i < in.length(); ++i) {
    char c = in[i];
    if (c == '"') out += '"';
    out += c;
  }
  out += "\"";
  return out;
}

void OsintLogger::logSession(const char* note) {
  if (!ready_) return;
  rotateIfNeeded_();
  File f = LittleFS.open(SCAN_LOG_PATH, "a");
  if (!f) return;
  f.printf("session,%lu,%s\n", (unsigned long)millis(), note ? note : "");
  f.close();
}

void OsintLogger::logWifi(const ScannedAP& ap) {
  if (!ready_) return;
  rotateIfNeeded_();
  File f = LittleFS.open(SCAN_LOG_PATH, "a");
  if (!f) return;
  f.print(F("wifi,"));
  f.print((unsigned long)millis());       f.print(',');
  f.print(csvEscape(ap.ssid));            f.print(',');
  f.print(ap.bssidStr);                   f.print(',');
  f.print(ap.rssi);                       f.print(',');
  f.print(ap.channel);                    f.print(',');
  f.print(WiFiScanner::authModeStr(ap.encryption)); f.print(',');
  f.print(csvEscape(ap.vendor));          f.print(',');
  f.print(ap.isKnown ? "yes" : "no");     f.print(',');
  f.print(csvEscape(ap.country));         f.print(',');
  f.println(csvEscape(ap.city));
  f.close();
}
