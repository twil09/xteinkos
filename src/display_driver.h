// ============================================================================
//  display_driver.h — thin wrapper around the GxEPD2 panel
// ----------------------------------------------------------------------------
//  Everything in the UI layer draws through here so the rest of the firmware
//  never touches GxEPD2 template types directly. Drawing happens inside a
//  paged render loop (paint callback), which is correct for GxEPD2 whether or
//  not a full framebuffer is available.
// ============================================================================
#pragma once

#include <Arduino.h>
#include <functional>
#include "display_config.h"

class DisplayDriver {
 public:
  using PaintFn = std::function<void(DisplayType&)>;

  // Initialises SPI + the panel. Call once in setup().
  bool begin();

  // Full-screen white clear.
  void clearScreen();

  // Runs `paint` inside GxEPD2's full-window page loop and pushes to the panel.
  void render(const PaintFn& paint);

  // Low-power state before deep sleep (keeps the current image on screen).
  void hibernate();

  DisplayType& gfx() { return display_; }
  int16_t width()  { return display_.width(); }
  int16_t height() { return display_.height(); }

 private:
  DisplayType display_{X3_PANEL_CLASS(EPD_PIN_CS, EPD_PIN_DC, EPD_PIN_RST,
                                      EPD_PIN_BUSY)};
  bool ready_ = false;
};
