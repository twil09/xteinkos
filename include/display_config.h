// ============================================================================
//  display_config.h — e-ink panel selection, pins and fonts
// ----------------------------------------------------------------------------
//  This is the ONE place you configure display hardware. Everything else in
//  the firmware talks to the panel through DisplayDriver, so if your X3 uses a
//  different controller you only edit this file.
// ============================================================================
#pragma once

#include <GxEPD2_BW.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>

#include "config.h"

// ---------------------------------------------------------------------------
//  >>> HARDWARE: X3 PANEL <<<
//
//  The Xteink X3 uses an SSD1677 controller driving a 3.68" 792x528 panel
//  (framebuffer 99x528 bytes = 52,272 bytes; the glass is physically 792x600).
//  GxEPD2 has NO stock class for 792x528, so a geometrically-correct image
//  requires a CUSTOM GxEPD2 class based on GxEPD2's SSD1677 driver
//  (GxEPD2_426_GDEQ0426T82) with WIDTH=792, HEIGHT=528, a 10 MHz SPI limit,
//  and the X3-specific LUT waveforms.
//
//  Until you drop that custom class in, X3_PANEL_CLASS defaults to the stock
//  SSD1677 4.26" class so the project COMPILES and talks to the correct
//  controller family — but the image geometry will be wrong (it is 800x480,
//  not 792x528). Replace it with your custom 792x528 SSD1677 class for a
//  correct picture, then set X3_PANEL_CLASS to that class name.
//
//  References for building the custom driver:
//    - papyrix-reader/docs/ssd1677-driver.md  (pinout, SPI, LUT overview)
//    - papyrix-reader/docs/x3-lut-waveforms.md (X3 LUT registers 0x20-0x24)
//    - CrazyCoder gist 82fec0bbd0e515dcc237d3db7451ec6f (RE analysis)
//    - ZinggJM/GxEPD2 src for GxEPD2_426_GDEQ0426T82.{h,cpp} (base to copy)
//
//  Layout throughout the firmware uses DISPLAY_WIDTH x DISPLAY_HEIGHT
//  (792 x 528 from the build flags).
// ---------------------------------------------------------------------------
#define X3_PANEL_CLASS GxEPD2_426_GDEQ0426T82

// SPI + control pins for the Xteink X3 (ESP32-C3). Confirmed from the SSD1677
// driver notes / RE analysis; re-verify if your board revision differs.
//   SPI: MSB first, mode 0, 10 MHz MAX (the X3 controller will not tolerate
//   faster — the stock 4.26" class clocks at 40 MHz, so your custom X3 class
//   must lower this).
#define EPD_PIN_SCK    8
#define EPD_PIN_MOSI   10
#define EPD_PIN_MISO   -1   // e-ink is write-only; leave unconnected
#define EPD_PIN_CS     21
#define EPD_PIN_DC     4
#define EPD_PIN_RST    5
#define EPD_PIN_BUSY   6

// Concrete display type used everywhere. The second template parameter is the
// number of rows buffered at once; the panel HEIGHT gives a full framebuffer.
using DisplayType = GxEPD2_BW<X3_PANEL_CLASS, X3_PANEL_CLASS::HEIGHT>;

// ---------------------------------------------------------------------------
//  Fonts — Adafruit GFX FreeFonts (baseline-positioned).
//  The spec asks for 28/18/14/12px tiers; the closest FreeFont sizes are used.
// ---------------------------------------------------------------------------
#define FONT_LARGE   (&FreeSansBold12pt7b)  // ~24px caps — header
#define FONT_MEDIUM  (&FreeSansBold9pt7b)   // labels / SSID
#define FONT_NORMAL  (&FreeSans9pt7b)       // body text
#define FONT_SMALL   (&FreeSans9pt7b)       // smallest FreeFont available

// Approximate cap-heights (px) for baseline math when placing text.
#define FONT_LARGE_CAP   17
#define FONT_MEDIUM_CAP  13
#define FONT_NORMAL_CAP  13
#define FONT_SMALL_CAP   13
