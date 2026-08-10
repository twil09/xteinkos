// theme.h — Duet-styled drawing helpers over a 1bpp GFX canvas.
#pragma once
#include <Adafruit_GFX.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include "config.h"

#define FONT_LARGE (&FreeSansBold12pt7b)  // ~17px cap — headers
#define FONT_MED   (&FreeSansBold9pt7b)   // list rows / labels
#define FONT_BODY  (&FreeSans9pt7b)       // body text

namespace duet {

inline void text(GFXcanvas1& g, int x, int baselineY, const GFXfont* f,
                 const String& s, uint16_t color) {
  g.setFont(f);
  g.setTextColor(color);
  g.setCursor(x, baselineY);
  g.print(s);
}

inline int textWidth(GFXcanvas1& g, const GFXfont* f, const String& s) {
  g.setFont(f);
  int16_t bx, by;
  uint16_t bw, bh;
  g.getTextBounds(s, 0, 0, &bx, &by, &bw, &bh);
  return (int)bw;
}

inline void centerText(GFXcanvas1& g, int cx, int baselineY, const GFXfont* f,
                       const String& s, uint16_t color) {
  text(g, cx - textWidth(g, f, s) / 2, baselineY, f, s, color);
}

inline String fit(GFXcanvas1& g, const GFXfont* f, const String& s, int maxW) {
  if (textWidth(g, f, s) <= maxW) return s;
  String t = s;
  while (t.length() > 1) {
    t.remove(t.length() - 1);
    if (textWidth(g, f, t + "...") <= maxW) return t + "...";
  }
  return String("...");
}

inline void header(GFXcanvas1& g, const String& title) {
  g.fillRect(0, 0, SCREEN_W, UI_HEADER_H, UI_BLACK);
  text(g, UI_MARGIN, (UI_HEADER_H + 17) / 2 + 3, FONT_LARGE,
       fit(g, FONT_LARGE, title, SCREEN_W - 2 * UI_MARGIN), UI_WHITE);
}

inline void footerHint(GFXcanvas1& g, const String& hint) {
  int y = SCREEN_H - UI_FOOTER_H;
  g.drawFastHLine(0, y, SCREEN_W, UI_BLACK);
  text(g, UI_MARGIN, y + 23, FONT_BODY,
       fit(g, FONT_BODY, hint, SCREEN_W - 2 * UI_MARGIN), UI_BLACK);
}

// Header bar with an optional right-aligned status (battery/wifi/etc).
inline void headerBar(GFXcanvas1& g, const String& title, const String& right) {
  g.fillRect(0, 0, SCREEN_W, UI_HEADER_H, UI_BLACK);
  text(g, UI_MARGIN, (UI_HEADER_H + 17) / 2 + 3, FONT_LARGE, title, UI_WHITE);
  if (right.length()) {
    int w = textWidth(g, FONT_BODY, right);
    text(g, SCREEN_W - UI_MARGIN - w, (UI_HEADER_H) / 2 + 6, FONT_BODY, right, UI_WHITE);
  }
}

// biscuit-style bottom control bar: 4 labelled buttons (empty = skip).
inline void buttonBar(GFXcanvas1& g, const char* a, const char* b,
                      const char* c, const char* d) {
  int y = SCREEN_H - UI_FOOTER_H;
  g.drawFastHLine(0, y, SCREEN_W, UI_BLACK);
  const char* labels[4] = {a, b, c, d};
  int bw = SCREEN_W / 4;
  for (int i = 0; i < 4; ++i) {
    if (!labels[i] || !labels[i][0]) continue;
    int x = i * bw;
    g.drawRoundRect(x + 6, y + 5, bw - 12, UI_FOOTER_H - 9, 4, UI_BLACK);
    centerText(g, x + bw / 2, y + UI_FOOTER_H / 2 + 6, FONT_BODY, labels[i], UI_BLACK);
  }
}

// A home section tile (biscuit style): title, subtitle, corner count.
inline void tile(GFXcanvas1& g, int x, int y, int w, int h, const String& title,
                 const String& sub, const String& count, bool selected) {
  uint16_t bg = selected ? UI_BLACK : UI_WHITE;
  uint16_t fg = selected ? UI_WHITE : UI_BLACK;
  g.fillRect(x, y, w, h, bg);
  g.drawRect(x, y, w, h, UI_BLACK);
  text(g, x + 16, y + 32, FONT_MED, fit(g, FONT_MED, title, w - 32), fg);
  if (sub.length())
    text(g, x + 16, y + 56, FONT_BODY, fit(g, FONT_BODY, sub, w - 32), fg);
  if (count.length()) {
    int cw = textWidth(g, FONT_BODY, count);
    text(g, x + w - 16 - cw, y + h - 14, FONT_BODY, count, fg);
  }
}

// A selectable row: filled black w/ white text when selected, else outlined.
inline void listRow(GFXcanvas1& g, int x, int y, int w, int h,
                    const String& label, bool selected) {
  if (selected) {
    g.fillRect(x, y, w, h, UI_BLACK);
  } else {
    g.fillRect(x, y, w, h, UI_WHITE);
    g.drawFastHLine(x, y + h - 1, w, UI_BLACK);
  }
  text(g, x + 16, y + h / 2 + 7, FONT_MED,
       fit(g, FONT_MED, label, w - 32), selected ? UI_WHITE : UI_BLACK);
}

}  // namespace duet
