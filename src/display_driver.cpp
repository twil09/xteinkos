#include "display_driver.h"

#include <SPI.h>

bool DisplayDriver::begin() {
  // Remap the hardware SPI bus to the e-ink pins (ESP32 supports remapping).
  SPI.end();
  SPI.begin(EPD_PIN_SCK, EPD_PIN_MISO, EPD_PIN_MOSI, EPD_PIN_CS);

  display_.init(115200, /*initial=*/true, /*reset_duration=*/20,
                /*pulldown_rst_mode=*/false);
  display_.setRotation(0);
  display_.setTextWrap(false);
  ready_ = true;

  Serial.printf("[disp] init %dx%d (layout target %dx%d)\n",
                display_.width(), display_.height(),
                DISPLAY_WIDTH, DISPLAY_HEIGHT);
  return ready_;
}

void DisplayDriver::clearScreen() {
  if (!ready_) return;
  display_.setFullWindow();
  display_.firstPage();
  do {
    display_.fillScreen(COLOR_BG);
  } while (display_.nextPage());
}

void DisplayDriver::render(const PaintFn& paint) {
  if (!ready_ || !paint) return;
  display_.setFullWindow();
  display_.firstPage();
  do {
    display_.fillScreen(COLOR_BG);
    paint(display_);
  } while (display_.nextPage());
}

void DisplayDriver::hibernate() {
  if (!ready_) return;
  display_.hibernate();
}
