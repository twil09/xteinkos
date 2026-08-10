// QrView — render a QR code for a string onto the 1bpp canvas (ricmoo/QRCode).
#pragma once
#include <qrcode.h>
#include <Adafruit_GFX.h>
#include "config.h"

// ricmoo's qrcode_getBufferSize() is a runtime function, so size the static
// buffer at compile time with the same formula (side = 4*ver+17).
#define QRV_MAXVER  8
#define QRV_SIDE(v) (4 * (v) + 17)
#define QRV_BUFSZ(v) ((QRV_SIDE(v) * QRV_SIDE(v) + 7) / 8)

namespace qrview {

inline uint8_t pickVer(size_t len) {
  static const uint16_t cap[] = {0, 17, 32, 53, 78, 106, 134, 154, 192};
  for (uint8_t v = 1; v <= QRV_MAXVER; ++v)
    if (len <= cap[v]) return v;
  return QRV_MAXVER;
}

// Draw centered horizontally, top at topY, sized to fit budgetPx. Returns the
// pixel side length (0 on failure).
inline int draw(GFXcanvas1& g, const String& text, int topY, int budgetPx) {
  static uint8_t buf[QRV_BUFSZ(QRV_MAXVER)];
  QRCode qr;
  uint8_t ver = pickVer(text.length());
  if (qrcode_initText(&qr, buf, ver, ECC_LOW,
                      const_cast<char*>(text.c_str())) != 0)
    return 0;
  int module = budgetPx / qr.size;
  if (module < 1) module = 1;
  if (module > 8) module = 8;
  int px = qr.size * module;
  int x0 = (SCREEN_W - px) / 2;
  for (uint8_t my = 0; my < qr.size; ++my)
    for (uint8_t mx = 0; mx < qr.size; ++mx)
      if (qrcode_getModule(&qr, mx, my))
        g.fillRect(x0 + mx * module, topY + my * module, module, module, UI_BLACK);
  return px;
}

}  // namespace qrview
