#include "DuetDisplay.h"

DuetDisplay::DuetDisplay()
    : eink_(EPD_SCLK, EPD_MOSI, EPD_CS, EPD_DC, EPD_RST, EPD_BUSY),
      canvas_(PANEL_NATIVE_W, PANEL_NATIVE_H) {}  // native landscape buffer

void DuetDisplay::begin() {
  eink_.setDisplayX3();       // 792x528 native geometry + X3 waveforms
  eink_.begin();              // inits SPI (pins from ctor) + panel
  canvas_.setRotation(EPD_ROTATION);  // draw in portrait 528x792
  clear();
}

void DuetDisplay::present(EInkDisplay::RefreshMode mode) {
  // GFXcanvas1 and EInkDisplay share the 1bpp layout (1=white, 0=black,
  // 99 bytes/row, MSB first), so the buffer copies straight across.
  eink_.setFramebuffer(canvas_.getBuffer());
  eink_.displayBuffer(mode);
}
