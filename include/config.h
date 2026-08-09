// ============================================================================
//  config.h — Global constants for X3 OSINT OS
// ----------------------------------------------------------------------------
//  Colors, sizes, timeouts and other tunables live here. Anything the display
//  layer needs that pulls in GxEPD2 / Adafruit_GFX headers lives in
//  display_config.h instead, so light-weight modules (config, http, portal)
//  can include this file without dragging in the whole graphics stack.
// ============================================================================
#pragma once

#include <stdint.h>
#include <string.h>

// ---------------------------------------------------------------------------
//  Firmware identity
// ---------------------------------------------------------------------------
#define FW_NAME     "X3 OSINT OS"
#define FW_VERSION  "0.1.0"

// ---------------------------------------------------------------------------
//  Display geometry (authoritative for layout)
//  These come from the build flags but are given safe defaults here so the
//  file is also usable by host-side tooling / editors.
// ---------------------------------------------------------------------------
#ifndef DISPLAY_WIDTH
#define DISPLAY_WIDTH  792
#endif
#ifndef DISPLAY_HEIGHT
#define DISPLAY_HEIGHT 528
#endif

// ---------------------------------------------------------------------------
//  Duet color palette
//  The X3 panel is black/white, so on-device we collapse the palette to ink
//  (black) and background (white). The original RGB values from the Duet
//  design language are kept as DUET_RGB_* for the HTML captive portal (which
//  renders in real color) and for any future color panel.
//  COLOR_* values match GxEPD_BLACK (0x0000) / GxEPD_WHITE (0xFFFF) so we do
//  not need to include GxEPD2 here.
// ---------------------------------------------------------------------------
static const uint16_t COLOR_INK = 0x0000;  // black  — text, outlines, dark header
static const uint16_t COLOR_BG  = 0xFFFF;  // white  — page + card background

// Conceptual RGB palette (used by the HTML portal, see web/portal.html)
#define DUET_RGB_BG       "#F5F5F0"  // Off-white background
#define DUET_RGB_TEXT     "#2C3E50"  // Dark gray text
#define DUET_RGB_ACCENT   "#3498DB"  // Blue accent
#define DUET_RGB_LIGHT    "#ECF0F1"  // Light gray (cards)
#define DUET_RGB_SUCCESS  "#27AE60"  // Green (known / success)
#define DUET_RGB_WARNING  "#E67E22"  // Orange (unknown / warning)

// ---------------------------------------------------------------------------
//  Layout metrics (pixels)
// ---------------------------------------------------------------------------
#define UI_MARGIN        20
#define UI_HEADER_H      60
#define UI_FOOTER_H      150   // reserved strip for QR code + status
#define UI_CARD_H        96
#define UI_CARD_GAP      12
#define UI_BADGE_W       120
#define UI_BADGE_H       30

// ---------------------------------------------------------------------------
//  Captive portal / soft-AP
// ---------------------------------------------------------------------------
#define AP_SSID          "X3-OSINT"
#define AP_PASSWORD      "password123"   // >= 8 chars for WPA2; set "" for open AP
#define AP_DNS_PORT      53
#define AP_HTTP_PORT     80

// ---------------------------------------------------------------------------
//  Filesystem paths
// ---------------------------------------------------------------------------
#define CONFIG_PATH        "/config.json"
#define DEFAULT_CONFIG_PATH "/default_config.json"
#define KNOWN_NETS_PATH    "/known_networks.txt"  // optional, one SSID or BSSID per line
#define OUI_DB_PATH        "/oui.csv"             // optional extended OUI table on FS
#define SCAN_LOG_PATH      "/scans.log"

// ---------------------------------------------------------------------------
//  Timeouts / intervals
// ---------------------------------------------------------------------------
#define HTTP_CONNECT_TIMEOUT_MS  5000
#define HTTP_READ_TIMEOUT_MS     8000
#define WIFI_CONNECT_TIMEOUT_MS  20000
#define WIFI_RETRY_DELAY_MS      30000
#define DEFAULT_SCAN_INTERVAL_S  300     // deep-sleep between scans (5 min)
#define SCAN_LOG_MAX_BYTES       (256 * 1024)  // rotate log past this size

// ---------------------------------------------------------------------------
//  Scanning
// ---------------------------------------------------------------------------
#define MAX_SCAN_RESULTS   32   // hard cap on APs we keep per cycle
#define MAX_DISPLAY_CARDS  3    // WiFi cards shown per e-ink frame

// ---------------------------------------------------------------------------
//  Hardware pins (NON-display).  Display + SPI pins live in display_config.h.
//  VERIFY THESE AGAINST YOUR X3 WIRING.
// ---------------------------------------------------------------------------
#define PIN_FORCE_PORTAL   9    // BOOT button on most ESP32-C3 boards (active LOW)
#define FORCE_PORTAL_ACTIVE_LEVEL 0

// ---------------------------------------------------------------------------
//  Reconnaissance mode.  Stored as a string in config.json; parsed to enum.
// ---------------------------------------------------------------------------
enum class OsintMode : uint8_t {
  PassiveScan = 0,  // WiFi scan only, no network calls
  ActiveScan  = 1,  // WiFi scan (BLE reserved for a future release)
  OsintMode   = 2   // WiFi scan + HTTP enrichment (GeoIP, vendor, known-net)
};

inline OsintMode parseMode(const char* s) {
  if (!s) return OsintMode::OsintMode;
  // Accept the portal's human labels and short tokens.
  if (strcmp(s, "Passive Scan") == 0 || strcmp(s, "passive") == 0)
    return OsintMode::PassiveScan;
  if (strcmp(s, "Active Scan") == 0 || strcmp(s, "active") == 0)
    return OsintMode::ActiveScan;
  return OsintMode::OsintMode;  // "OSINT Mode" / "osint" / default
}

inline const char* modeToken(OsintMode m) {
  switch (m) {
    case OsintMode::PassiveScan: return "passive";
    case OsintMode::ActiveScan:  return "active";
    default:                     return "osint";
  }
}
