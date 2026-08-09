// ============================================================================
//  display_config.h — e-ink panel selection, pins and fonts
// ----------------------------------------------------------------------------
//  This is the ONE place you configure display hardware. Everything else in
//  the firmware talks to the panel through DisplayDriver, so if your X3 uses a
//  different controller you only edit this file.
// ============================================================================
#pragma once

#include <GxEPD2_BW.h>
#include <GxEPD2_368_X3.h>   // custom SSD1677 792x528 class (lib/GxEPD2_X3)
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>

#include "config.h"

// ---------------------------------------------------------------------------
//  >>> HARDWARE: X3 PANEL <<<
//
//  The Xteink X3 uses an SSD1677 controller driving a 3.68" 792x528 panel
//  (framebuffer 99x528 bytes = 52,272 bytes; the glass is physically 792x600).
//  GxEPD2 has no stock class for 792x528, so this project ships a custom one:
//  lib/GxEPD2_X3/GxEPD2_368_X3 — a faithful adaptation of GxEPD2's SSD1677
//  driver (GDEQ0426T82) at 792x528, using the panel's built-in OTP waveforms
//  (no custom LUT upload needed for full refresh). SPI defaults to 4 MHz mode 0
//  in GxEPD2, safely under the X3's 10 MHz ceiling.
//
//  This is written from the confirmed X3 specs but has NOT been validated on
//  real hardware. If the image is wrong, see the tuning notes at the top of
//  lib/GxEPD2_X3/GxEPD2_368_X3.h. To fall back to the stock 4.26" SSD1677 class
//  for bring-up (right controller, wrong 800x480 geometry), set X3_PANEL_CLASS
//  to GxEPD2_426_GDEQ0426T82 instead.
//
//  Layout throughout the firmware uses DISPLAY_WIDTH x DISPLAY_HEIGHT
//  (792 x 528 from the build flags).
// ---------------------------------------------------------------------------
#define X3_PANEL_CLASS GxEPD2_368_X3

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
