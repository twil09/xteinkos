#include "osint_db.h"

#include <LittleFS.h>

// ---------------------------------------------------------------------------
//  Built-in OUI table (first 3 bytes of the MAC -> vendor).
//  A tiny curated set of common consumer/AP vendors. For full coverage drop an
//  IEEE OUI export at /oui.csv on the LittleFS partition.
// ---------------------------------------------------------------------------
namespace {
struct OuiEntry {
  uint32_t    oui;      // 0x00AABBCC
  const char* vendor;
};

const OuiEntry kOui[] = {
    {0x001A2B, "Ayecom"},
    {0x002556, "Cisco"},
    {0x0018F8, "Cisco-Linksys"},
    {0x001C10, "Cisco-Linksys"},
    {0x00236C, "Apple"},
    {0x002608, "Apple"},
    {0x3C0754, "Apple"},
    {0xF0189E, "Apple"},
    {0xA45E60, "Apple"},
    {0xDC2B2A, "Apple"},
    {0x001D0F, "TP-Link"},
    {0x50C7BF, "TP-Link"},
    {0xB0487A, "TP-Link"},
    {0xEC086B, "TP-Link"},
    {0x14CC20, "TP-Link"},
    {0x001E2A, "Netgear"},
    {0x9C3DCF, "Netgear"},
    {0xA040A0, "Netgear"},
    {0x00095B, "Netgear"},
    {0x001B2F, "Netgear"},
    {0x00223F, "Netgear"},
    {0x0024B2, "Netgear"},
    {0x000D88, "D-Link"},
    {0x1CBDB9, "D-Link"},
    {0x340804, "D-Link"},
    {0x001CF0, "D-Link"},
    {0x0026F2, "Netgear"},
    {0x002401, "D-Link"},
    {0x001CDF, "Belkin"},
    {0x08863B, "Belkin"},
    {0x944452, "Belkin"},
    {0x0018E7, "Cameo/Belkin"},
    {0x00037F, "Atheros"},
    {0x000C43, "Ralink"},
    {0x00E04C, "Realtek"},
    {0x52540A, "Realtek(virt)"},
    {0x001759, "Huawei"},
    {0x0025CB, "Huawei"},
    {0x28F3B9, "Huawei"},
    {0x88A2D7, "Huawei"},
    {0xAC853D, "Huawei"},
    {0x001632, "Samsung"},
    {0x0021D1, "Samsung"},
    {0x5CF6DC, "Samsung"},
    {0x8425DB, "Samsung"},
    {0xA00BBA, "Samsung"},
    {0x001377, "Samsung"},
    {0xFCF528, "Zyxel"},
    {0x001349, "Zyxel"},
    {0xB0B2DC, "Zyxel"},
    {0x00904C, "Epigram/Broadcom"},
    {0x001018, "Broadcom"},
    {0x8C1F64, "IEEE-Registration"},
    {0x001A11, "Google"},
    {0x3C5AB4, "Google"},
    {0xF4F5E8, "Google"},
    {0xDA9048, "Google(Nest)"},
    {0x08B4B1, "Google"},
    {0xB827EB, "Raspberry Pi"},
    {0xDCA632, "Raspberry Pi"},
    {0xE45F01, "Raspberry Pi"},
    {0x2CCF67, "Raspberry Pi"},
    {0x24628C, "Espressif"},
    {0x240AC4, "Espressif"},
    {0x30AEA4, "Espressif"},
    {0x7CDFA1, "Espressif"},
    {0x8CAAB5, "Espressif"},
    {0xA4CF12, "Espressif"},
    {0xB4E62D, "Espressif"},
    {0xC8C9A3, "Espressif"},
    {0xEC64C9, "Espressif"},
    {0x001B63, "Apple"},
    {0x60334B, "Apple"},
    {0x001EC2, "Apple"},
    {0x0022FB, "Intel"},
    {0x001B21, "Intel"},
    {0x7C7A91, "Intel"},
    {0x001DE0, "Intel"},
    {0x00166F, "Intel"},
    {0xFCF8AE, "Intel"},
};
constexpr size_t kOuiCount = sizeof(kOui) / sizeof(kOui[0]);
}  // namespace

bool OsintDB::begin() {
  hasCsv_ = LittleFS.exists(OUI_DB_PATH);

  knownList_.clear();
  if (LittleFS.exists(KNOWN_NETS_PATH)) {
    File f = LittleFS.open(KNOWN_NETS_PATH, "r");
    if (f) {
      while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.length() == 0 || line.startsWith("#")) continue;
        line.toUpperCase();
        knownList_.push_back(line);
      }
      f.close();
    }
  }
  Serial.printf("[db] built-in OUI=%u, csv=%s, known=%u\n",
                (unsigned)kOuiCount, hasCsv_ ? "yes" : "no",
                (unsigned)knownList_.size());
  return true;
}

uint32_t OsintDB::ouiFromBytes_(const uint8_t b[6]) {
  return ((uint32_t)b[0] << 16) | ((uint32_t)b[1] << 8) | (uint32_t)b[2];
}

String OsintDB::bssidToString_(const uint8_t b[6]) {
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
           b[0], b[1], b[2], b[3], b[4], b[5]);
  return String(buf);
}

String OsintDB::lookupMAC(const uint8_t bssid[6]) {
  uint32_t oui = ouiFromBytes_(bssid);
  for (size_t i = 0; i < kOuiCount; ++i) {
    if (kOui[i].oui == oui) return String(kOui[i].vendor);
  }
  if (hasCsv_) {
    String v = lookupCsv_(oui);
    if (v.length()) return v;
  }
  return String("Unknown");
}

String OsintDB::lookupMAC(const String& macStr) {
  // Accept "AA:BB:CC:.." or "AABBCC..". We only need the first 3 bytes.
  uint8_t b[3] = {0, 0, 0};
  int got = 0;
  unsigned int cur = 0;
  int nib = 0;
  for (size_t i = 0; i < macStr.length() && got < 3; ++i) {
    char c = macStr[i];
    int val;
    if (c >= '0' && c <= '9') val = c - '0';
    else if (c >= 'a' && c <= 'f') val = c - 'a' + 10;
    else if (c >= 'A' && c <= 'F') val = c - 'A' + 10;
    else continue;  // skip ':' '-' etc.
    cur = (cur << 4) | val;
    if (++nib == 2) {
      b[got++] = (uint8_t)cur;
      cur = 0;
      nib = 0;
    }
  }
  uint8_t full[6] = {b[0], b[1], b[2], 0, 0, 0};
  return lookupMAC(full);
}

String OsintDB::lookupCsv_(uint32_t oui) {
  File f = LittleFS.open(OUI_DB_PATH, "r");
  if (!f) return String();

  char target[7];
  snprintf(target, sizeof(target), "%06X", (unsigned)oui);

  String result;
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() < 8) continue;
    int comma = line.indexOf(',');
    if (comma < 6) continue;
    String prefix = line.substring(0, comma);
    prefix.replace(":", "");
    prefix.replace("-", "");
    prefix.toUpperCase();
    if (prefix == target) {
      result = line.substring(comma + 1);
      result.trim();
      break;
    }
  }
  f.close();
  return result;
}

bool OsintDB::isKnown(const String& ssid, const uint8_t bssid[6]) {
  if (homeSsid_.length() && ssid == homeSsid_) return true;

  String ssidU = ssid;
  ssidU.toUpperCase();
  String bssidU = bssidToString_(bssid);
  bssidU.toUpperCase();

  for (const String& k : knownList_) {
    if (k == ssidU || k == bssidU) return true;
  }
  return false;
}
