// ============================================================================
//  config.h — global constants for the X3 Duet OS
// ============================================================================
#pragma once
#include <stdint.h>

#define FW_NAME     "Vix OS"
#define FW_VERSION  "0.4.0"

// ---- Display geometry (X3 in PORTRAIT) -------------------------------------
//  Panel is 792x528 native (landscape); we rotate the canvas 90 deg so the UI
//  is portrait 528x792. Named SCREEN_W/SCREEN_H to avoid colliding with
//  EInkDisplay's DISPLAY_WIDTH/DISPLAY_HEIGHT members.
#define SCREEN_W  528
#define SCREEN_H  792
#define PANEL_NATIVE_W 792   // physical panel is landscape; canvas is rotated
#define PANEL_NATIVE_H 528
#define EPD_ROTATION 3   // 1 or 3 (flip if the image comes out upside-down)

// ---- E-ink pins (confirmed X3 wiring; matches CrossPoint HAL) --------------
#define EPD_SCLK  8
#define EPD_MOSI  10
#define EPD_CS    21
#define EPD_DC    4
#define EPD_RST   5
#define EPD_BUSY  6

// microSD shares the SPI bus (SCLK=8, MOSI=10) with the e-ink; CS=12, MISO=7.
// The e-ink driver inits SPI with MISO disabled, so we re-enable MISO for SD.
#define PIN_SD_MISO 7

// ---- Power / sleep ---------------------------------------------------------
#define PIN_POWER       3        // digital power button (active LOW), wake source
#define IDLE_SLEEP_MS   120000   // deep-sleep after this long with no activity

// ---- Battery (X3 board profile: ADC on GPIO0, 2.0x divider) ----------------
#define BATTERY_ADC_PIN 0
#define BATTERY_DIVIDER 2.0f

// ---- Networking ------------------------------------------------------------
#define MDNS_HOST   "vix"        // reach the device at http://vix.local
#define FILE_PORT   80           // file-transfer / setup web server port
#define TZ_PATH     "/tz"        // persisted UTC offset in minutes

// ---- 1bpp canvas colors (GFXcanvas1 / EInkDisplay: 1 = white, 0 = black) ---
static const uint16_t UI_WHITE = 1;  // background
static const uint16_t UI_BLACK = 0;  // ink

// ---- Duet palette (real RGB, for the HTML captive portal only) -------------
#define DUET_RGB_BG      "#F5F5F0"
#define DUET_RGB_TEXT    "#2C3E50"
#define DUET_RGB_ACCENT  "#3498DB"
#define DUET_RGB_LIGHT   "#ECF0F1"

// ---- Layout metrics --------------------------------------------------------
#define UI_MARGIN     20
#define UI_HEADER_H   62
#define UI_FOOTER_H   34

// ---- Filesystem paths ------------------------------------------------------
#define SETTINGS_PATH "/settings.json"
#define WIFI_PATH     "/wifi.json"
#define STATS_PATH    "/stats.json"   // reading time + speed + completion

// ---- Button indices (from the SDK InputManager) ----------------------------
//  ADC ladder 1 -> Back/Confirm/Left/Right, ladder 2 -> Up/Down, plus Power.
enum class Btn : uint8_t {
  Back = 0, Confirm = 1, Left = 2, Right = 3, Up = 4, Down = 5, Power = 6, None = 255
};
#define BTN_COUNT 7
