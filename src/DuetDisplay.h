// DuetDisplay — draws into a 1bpp GFX canvas and pushes it to the X3 e-ink.
#pragma once
#include <Adafruit_GFX.h>
#include <EInkDisplay.h>
#include "config.h"

class DuetDisplay {
 public:
  DuetDisplay();
  void begin();

  GFXcanvas1& gfx() { return canvas_; }
  int width() const { return SCREEN_W; }
  int height() const { return SCREEN_H; }

  void clear() { canvas_.fillScreen(UI_WHITE); }
  void present(EInkDisplay::RefreshMode mode = EInkDisplay::FAST_REFRESH);

 private:
  EInkDisplay eink_;
  GFXcanvas1  canvas_;
};
