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
//  Buttons (Xteink X3).  Two resistor-ladder ADC lines + one digital power
//  button. Raw 12-bit ADC centers (0..4095) at 11 dB attenuation, confirmed
//  from X3/X4 firmware analysis. Display/SPI pins live in display_config.h.
//
//  Physical layout: two large paddles each split L/R (4 zones) on ADC_A, plus
//  two side buttons on ADC_B. Logical mapping (right=Up, left=Down):
//    Up     = ADC_A RIGHT  or ADC_B UP
//    Down   = ADC_A LEFT   or ADC_B DOWN
//    Select = ADC_A CONFIRM
//    Back   = ADC_A BACK
//    Power  = digital power button
// ---------------------------------------------------------------------------
#define BTN_ADC_PIN_A   1     // GPIO1: BACK / CONFIRM / LEFT / RIGHT
#define BTN_ADC_PIN_B   2     // GPIO2: UP / DOWN
#define BTN_POWER_PIN   3     // GPIO3: power button, active LOW

// ADC ladder centers (raw counts, 12-bit / 11 dB)
#define ADC_A_BACK      3512
#define ADC_A_CONFIRM   2694
#define ADC_A_LEFT      1493
#define ADC_A_RIGHT     5
#define ADC_B_UP        2242
#define ADC_B_DOWN      5
#define ADC_TOLERANCE   300   // +/- match window; idle line rests near 4095

#define BTN_DEBOUNCE_MS     20
#define BTN_LONGPRESS_MS    800   // power long-press = sleep now
#define BTN_REPEAT_MS       350   // held Up/Down auto-repeat

// Hold BACK during power-on to force the captive portal (reconfigure).
#define FORCE_PORTAL_HOLD_MS 1200

// ---------------------------------------------------------------------------
//  Interactivity / power management
// ---------------------------------------------------------------------------
#define UI_IDLE_SLEEP_MS   60000   // no input this long -> deep sleep
#define PIN_WAKE           BTN_POWER_PIN  // GPIO wake source from deep sleep
#define LAST_SCAN_PATH     "/last_scan.json"  // cached results for instant wake

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
