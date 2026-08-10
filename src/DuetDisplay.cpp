#include "DuetDisplay.h"

DuetDisplay::DuetDisplay()
    : eink_(EPD_SCLK, EPD_MOSI, EPD_CS, EPD_DC, EPD_RST, EPD_BUSY),
      canvas_(SCREEN_W, SCREEN_H) {}

void DuetDisplay::begin() {
  eink_.setDisplayX3();  // 792x528 geometry + X3 waveforms
  eink_.begin();         // inits SPI (pins from ctor) + panel
  clear();
}

void DuetDisplay::present(EInkDisplay::RefreshMode mode) {
  // GFXcanvas1 and EInkDisplay share the 1bpp layout (1=white, 0=black,
  // 99 bytes/row, MSB first), so the buffer copies straight across.
  eink_.setFramebuffer(canvas_.getBuffer());
  eink_.displayBuffer(mode);
}
