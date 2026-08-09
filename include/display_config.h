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
//  >>> HARDWARE: VERIFY / EDIT FOR YOUR X3 <<<
//
//  The Xteink X3's exact panel controller is not published, so we default to a
//  common large monochrome panel class that ships with GxEPD2 (Waveshare 7.5"
//  UC8179, 800x480). The firmware compiles and runs against it, but for a
//  correct image on YOUR device set X3_PANEL_CLASS to the GxEPD2 class that
//  matches your controller (see GxEPD2/src/GxEPD2_BW.h for the full list) and
//  adjust the pin map below to your wiring.
//
//  Layout throughout the firmware uses DISPLAY_WIDTH x DISPLAY_HEIGHT
//  (792 x 528 from the build flags), so once the right panel is selected the
//  UI lands in the right place.
// ---------------------------------------------------------------------------
#define X3_PANEL_CLASS GxEPD2_750_T7

// SPI + control pins — VERIFY against your board.
#define EPD_PIN_SCK    4
#define EPD_PIN_MOSI   6
#define EPD_PIN_MISO   -1   // e-ink is write-only; leave unconnected
#define EPD_PIN_CS     7
#define EPD_PIN_DC     5
#define EPD_PIN_RST    10
#define EPD_PIN_BUSY   3

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
