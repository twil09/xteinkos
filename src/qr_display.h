// ============================================================================
//  qr_display.h — QR code generation (ricmoo/QRCode) + e-ink rendering
// ----------------------------------------------------------------------------
//  Generate once (outside the page loop), then draw() as many times as the
//  paged refresh needs. Modules are drawn as filled black squares on the
//  white background; the surrounding quiet zone is left to the caller's layout.
// ============================================================================
#pragma once

#include <Arduino.h>
#include "qrcode.h"
#include "display_config.h"

// Largest QR version we allocate a buffer for. v8 (49x49) holds ~190 bytes in
// byte mode at ECC-LOW — comfortably more than any results URL we produce.
#define QR_MAX_VERSION 8

// Compile-time buffer size (ricmoo's qrcode_getBufferSize() is a runtime
// function, so we can't use it to size a static array). Mirrors its formula:
// side = 4*version + 17, bytes = ceil(side*side / 8).
#define QR_SIDE_(v)   (4 * (v) + 17)
#define QR_BUFSZ_(v)  ((QR_SIDE_(v) * QR_SIDE_(v) + 7) / 8)

class QRDisplay {
 public:
  // Encode `text`. Picks the smallest version that fits (up to QR_MAX_VERSION).
  // Returns false if the text is too long to encode.
  bool generate(const char* text);

  // Draw the last-generated code with the top-left module at (x,y).
  void draw(DisplayType& gfx, int x, int y, int moduleSize);

  // Pixel side length for a given module size (0 if nothing generated).
  int sizePixels(int moduleSize) const {
    return valid_ ? qrcode_.size * moduleSize : 0;
  }

  bool valid() const { return valid_; }

 private:
  QRCode  qrcode_{};
  uint8_t buffer_[QR_BUFSZ_(QR_MAX_VERSION)]{};
  bool    valid_ = false;

  static uint8_t pickVersion_(size_t len);
};
