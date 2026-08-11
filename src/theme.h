// theme.h — Duet-styled drawing helpers over a 1bpp GFX canvas.
#pragma once
#include <Adafruit_GFX.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include "Battery.h"
#include "Clock.h"
#include "Net.h"
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

inline void footerHint(GFXcanvas1& g, const String& hint) {
  int y = SCREEN_H - UI_FOOTER_H;
  g.drawFastHLine(0, y, SCREEN_W, UI_BLACK);
  text(g, UI_MARGIN, y + 23, FONT_BODY,
       fit(g, FONT_BODY, hint, SCREEN_W - 2 * UI_MARGIN), UI_BLACK);
}

// ---- Status-bar building blocks (drawn in `color` on the black header) ------

inline void thickLine(GFXcanvas1& g, int x0, int y0, int x1, int y1, int t,
                      uint16_t color) {
  for (int o = 0; o < t; ++o) {
    g.drawLine(x0 + o, y0, x1 + o, y1, color);
    g.drawLine(x0, y0 + o, x1, y1 + o, color);
  }
}

// The Vix "V" logo mark: rounded tile with a bold V. Returns its right edge.
inline int logoMark(GFXcanvas1& g, int x, int cy, int s, uint16_t fg,
                    uint16_t bg) {
  int y = cy - s / 2;
  g.fillRoundRect(x, y, s, s, 6, fg);
  int lx = x + s * 28 / 100, rx = x + s * 72 / 100, mx = x + s / 2;
  int ty = y + s * 26 / 100, by = y + s * 74 / 100;
  thickLine(g, lx, ty, mx, by, 3, bg);
  thickLine(g, rx, ty, mx, by, 3, bg);
  return x + s;
}

// Battery icon ending at rightX; returns its left x.
inline int batteryGlyph(GFXcanvas1& g, int rightX, int cy, int pct,
                       uint16_t color) {
  const int bw = 28, bh = 15, nub = 3;
  int x = rightX - nub - bw, y = cy - bh / 2;
  g.drawRect(x, y, bw, bh, color);
  g.fillRect(x + bw, y + (bh - 6) / 2, nub, 6, color);
  int inner = bw - 4, fw = inner * pct / 100;
  if (fw < 0) fw = 0;
  if (fw > inner) fw = inner;
  if (fw > 0) g.fillRect(x + 2, y + 2, fw, bh - 4, color);
  return x;
}

// Wi-Fi signal bars ending at rightX; filled when on, outlined when off.
inline int wifiGlyph(GFXcanvas1& g, int rightX, int cy, bool on,
                    uint16_t color) {
  const int barW = 4, gap = 2, n = 3, total = n * barW + (n - 1) * gap;
  int x = rightX - total, baseY = cy + 7;
  for (int i = 0; i < n; ++i) {
    int h = 5 + i * 4, bx = x + i * (barW + gap);
    if (on) g.fillRect(bx, baseY - h, barW, h, color);
    else g.drawRect(bx, baseY - h, barW, h, color);
  }
  return x;
}

// time · wifi · battery, right-aligned. Returns the leftmost x it used.
inline int statusCluster(GFXcanvas1& g, int cy, uint16_t color) {
  int R = SCREEN_W - UI_MARGIN;
  int pct = Battery::percent();
  int battLeft = batteryGlyph(g, R, cy, pct, color);
  String ps = String(pct) + "%";
  int pw = textWidth(g, FONT_BODY, ps);
  int psx = battLeft - 8 - pw;
  text(g, psx, cy + 6, FONT_BODY, ps, color);
  int wifiLeft = wifiGlyph(g, psx - 10, cy, Net::connected(), color);
  String tm = Clock::hhmm();
  int tw = textWidth(g, FONT_BODY, tm);
  int tx = wifiLeft - 12 - tw;
  text(g, tx, cy + 6, FONT_BODY, tm, color);
  return tx;
}

// Standard header: Vix logo + title on the left, status cluster on the right.
inline void headerBar(GFXcanvas1& g, const String& title, const String& right) {
  g.fillRect(0, 0, SCREEN_W, UI_HEADER_H, UI_BLACK);
  int cy = UI_HEADER_H / 2;
  int clusterLeft = statusCluster(g, cy, UI_WHITE);
  int rightEdge = clusterLeft;
  if (right.length()) {
    int w = textWidth(g, FONT_BODY, right);
    text(g, clusterLeft - 12 - w, cy + 6, FONT_BODY, right, UI_WHITE);
    rightEdge = clusterLeft - 12 - w;
  }
  int lx = logoMark(g, UI_MARGIN, cy, 34, UI_WHITE, UI_BLACK) + 12;
  int maxW = rightEdge - 10 - lx;
  text(g, lx, (UI_HEADER_H + 17) / 2 + 3, FONT_LARGE,
       fit(g, FONT_LARGE, title, maxW), UI_WHITE);
}

inline void header(GFXcanvas1& g, const String& title) {
  headerBar(g, title, "");
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
