#include "qr_display.h"

// Approximate byte-mode capacities at ECC-LOW per version (index = version).
uint8_t QRDisplay::pickVersion_(size_t len) {
  static const uint16_t cap[] = {0, 17, 32, 53, 78, 106, 134, 154, 192};
  for (uint8_t v = 1; v <= QR_MAX_VERSION; ++v) {
    if (len <= cap[v]) return v;
  }
  return QR_MAX_VERSION;  // caller checks the return of qrcode_initText anyway
}

bool QRDisplay::generate(const char* text) {
  valid_ = false;
  if (!text) return false;

  size_t len = strlen(text);
  if (len == 0) return false;

  uint8_t version = pickVersion_(len);
  int8_t rc = qrcode_initText(&qrcode_, buffer_, version, ECC_LOW,
                              const_cast<char*>(text));
  if (rc != 0) {
    Serial.printf("[qr] init failed (len=%u, ver=%u)\n", (unsigned)len, version);
    return false;
  }
  valid_ = true;
  return true;
}

void QRDisplay::draw(DisplayType& gfx, int x, int y, int moduleSize) {
  if (!valid_ || moduleSize < 1) return;
  for (uint8_t my = 0; my < qrcode_.size; ++my) {
    for (uint8_t mx = 0; mx < qrcode_.size; ++mx) {
      if (qrcode_getModule(&qrcode_, mx, my)) {
        gfx.fillRect(x + mx * moduleSize, y + my * moduleSize,
                     moduleSize, moduleSize, COLOR_INK);
      }
    }
  }
}
