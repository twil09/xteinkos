#include "duet_theme.h"

// ---------------------------------------------------------------------------
//  Small text helpers
// ---------------------------------------------------------------------------
void DuetTheme::textAt_(DisplayType& g, int x, int baselineY,
                        const GFXfont* font, const String& s, uint16_t color) {
  g.setFont(font);
  g.setTextColor(color);
  g.setCursor(x, baselineY);
  g.print(s);
}

String DuetTheme::fit_(DisplayType& g, const GFXfont* font, const String& s,
                       int maxW) {
  g.setFont(font);
  int16_t bx, by;
  uint16_t bw, bh;
  g.getTextBounds(s, 0, 0, &bx, &by, &bw, &bh);
  if ((int)bw <= maxW) return s;

  String t = s;
  while (t.length() > 1) {
    t.remove(t.length() - 1);
    String probe = t + "...";
    g.getTextBounds(probe, 0, 0, &bx, &by, &bw, &bh);
    if ((int)bw <= maxW) return probe;
  }
  return String("...");
}

void DuetTheme::drawBadge_(DisplayType& g, int x, int y, int w, int h,
                           const char* text, bool filled) {
  if (filled) {
    g.fillRoundRect(x, y, w, h, 4, COLOR_INK);
  } else {
    g.fillRoundRect(x, y, w, h, 4, COLOR_BG);
    g.drawRoundRect(x, y, w, h, 4, COLOR_INK);
  }
  uint16_t textColor = filled ? COLOR_BG : COLOR_INK;
  String label = text;
  g.setFont(FONT_SMALL);
  int16_t bx, by;
  uint16_t bw, bh;
  g.getTextBounds(label, 0, 0, &bx, &by, &bw, &bh);
  int tx = x + (w - (int)bw) / 2;
  int ty = y + (h + FONT_SMALL_CAP) / 2;
  textAt_(g, tx, ty, FONT_SMALL, label, textColor);
}

// ---------------------------------------------------------------------------
//  Frame composition
// ---------------------------------------------------------------------------
void DuetTheme::begin() {
  wifi_.clear();
  hasGeo_ = false;
  hasQr_ = false;
  status_ = "";
  statusErr_ = false;
  geoIp_ = geoCountry_ = geoCity_ = "";
  qrUrl_ = qrLabel_ = "";
}

void DuetTheme::setHeader(const String& title) { title_ = title; }

void DuetTheme::addWiFi(const ScannedAP& ap) { wifi_.push_back(ap); }

void DuetTheme::renderWiFiResults(const std::vector<ScannedAP>& nets) {
  for (const auto& n : nets) wifi_.push_back(n);
}

void DuetTheme::setGeoIP(const String& ip, const String& country,
                         const String& city) {
  geoIp_ = ip;
  geoCountry_ = country;
  geoCity_ = city;
  hasGeo_ = true;
}

void DuetTheme::setStatus(const String& status, bool isError) {
  status_ = status;
  statusErr_ = isError;
}

void DuetTheme::setQR(const String& url, const String& label) {
  qrUrl_ = url;
  qrLabel_ = label;
  hasQr_ = qr_.generate(url.c_str());  // encode once, outside the page loop
}

void DuetTheme::commit() {
  drv_.render([this](DisplayType& g) { paint_(g); });
  drv_.hibernate();
}

// ---------------------------------------------------------------------------
//  Painting (runs once per GxEPD2 page)
// ---------------------------------------------------------------------------
void DuetTheme::paintHeader_(DisplayType& g, int w) {
  g.fillRect(0, 0, w, UI_HEADER_H, COLOR_INK);
  int baseline = (UI_HEADER_H + FONT_LARGE_CAP) / 2;
  String t = fit_(g, FONT_LARGE, title_, w - 2 * UI_MARGIN);
  textAt_(g, UI_MARGIN, baseline, FONT_LARGE, t, COLOR_BG);
}

int DuetTheme::paintGeo_(DisplayType& g, int w, int y) {
  const int h = 56;
  g.drawRect(UI_MARGIN, y, w - 2 * UI_MARGIN, h, COLOR_INK);

  String line = "Location: ";
  if (geoCity_.length()) line += geoCity_;
  if (geoCity_.length() && geoCountry_.length()) line += ", ";
  if (geoCountry_.length()) line += geoCountry_;
  if (!geoCity_.length() && !geoCountry_.length()) line += "unknown";

  textAt_(g, UI_MARGIN + 12, y + 22, FONT_MEDIUM,
          fit_(g, FONT_MEDIUM, line, w - 2 * UI_MARGIN - 24), COLOR_INK);
  if (geoIp_.length()) {
    textAt_(g, UI_MARGIN + 12, y + 44, FONT_SMALL,
            fit_(g, FONT_SMALL, "Public IP: " + geoIp_,
                 w - 2 * UI_MARGIN - 24),
            COLOR_INK);
  }
  return y + h + UI_CARD_GAP;
}

void DuetTheme::paintWiFiCard_(DisplayType& g, int w, int y,
                               const ScannedAP& ap) {
  const int x = UI_MARGIN;
  const int cardW = w - 2 * UI_MARGIN;
  g.drawRect(x, y, cardW, UI_CARD_H, COLOR_INK);

  // Badge (top-right).
  const int badgeX = x + cardW - UI_BADGE_W - 12;
  drawBadge_(g, badgeX, y + 12, UI_BADGE_W, UI_BADGE_H,
             ap.isKnown ? "Known" : "Unknown", ap.isKnown);

  // SSID (leave room for the badge).
  int ssidMaxW = badgeX - (x + 15) - 10;
  textAt_(g, x + 15, y + 30, FONT_MEDIUM,
          fit_(g, FONT_MEDIUM, ap.ssid, ssidMaxW), COLOR_INK);

  // Detail line 1: signal + vendor.
  String l1 = "Signal: " + String(ap.rssi) + " dBm  |  " + ap.vendor;
  textAt_(g, x + 15, y + 56, FONT_SMALL,
          fit_(g, FONT_SMALL, l1, cardW - 30), COLOR_INK);

  // Detail line 2: bssid, channel, auth.
  String l2 = ap.bssidStr + "   Ch " + String(ap.channel) + "   " +
              WiFiScanner::authModeStr(ap.encryption);
  textAt_(g, x + 15, y + 80, FONT_SMALL,
          fit_(g, FONT_SMALL, l2, cardW - 30), COLOR_INK);
}

void DuetTheme::paintFooter_(DisplayType& g, int w, int h) {
  const int footY = h - UI_FOOTER_H;
  g.drawFastHLine(0, footY, w, COLOR_INK);

  // QR block on the right.
  int qrRight = w - UI_MARGIN;
  if (hasQr_) {
    // Pick the largest module size (1..4 px) whose code fits the footer height.
    int moduleSize = 4;
    const int budget = UI_FOOTER_H - 34;  // leave room for the label
    while (moduleSize > 1 && qr_.sizePixels(moduleSize) > budget) --moduleSize;
    int qrPx = qr_.sizePixels(moduleSize);
    int qx = w - UI_MARGIN - qrPx;
    int qy = footY + (UI_FOOTER_H - qrPx) / 2;
    if (qy < footY + 8) qy = footY + 8;
    qr_.draw(g, qx, qy, moduleSize);

    if (qrLabel_.length()) {
      textAt_(g, qx, qy + qrPx + 16, FONT_SMALL,
              fit_(g, FONT_SMALL, qrLabel_, qrPx + 40), COLOR_INK);
    }
    qrRight = qx - UI_MARGIN;
  }

  // Status text on the left.
  if (status_.length()) {
    String s = statusErr_ ? ("! " + status_) : status_;
    textAt_(g, UI_MARGIN, footY + 30, FONT_MEDIUM,
            fit_(g, FONT_MEDIUM, s, qrRight - UI_MARGIN), COLOR_INK);
  }
  // Firmware footer tag.
  textAt_(g, UI_MARGIN, h - 14, FONT_SMALL,
          String(FW_NAME) + " v" + FW_VERSION, COLOR_INK);
}

void DuetTheme::paint_(DisplayType& g) {
  const int w = g.width();
  const int h = g.height();

  paintHeader_(g, w);

  int y = UI_HEADER_H + UI_MARGIN;
  if (hasGeo_) y = paintGeo_(g, w, y);

  const int contentBottom = h - UI_FOOTER_H - UI_CARD_GAP;
  int shown = 0;
  for (const auto& ap : wifi_) {
    if (shown >= MAX_DISPLAY_CARDS) break;
    if (y + UI_CARD_H > contentBottom) break;
    paintWiFiCard_(g, w, y, ap);
    y += UI_CARD_H + UI_CARD_GAP;
    ++shown;
  }

  if (wifi_.empty()) {
    textAt_(g, UI_MARGIN, y + 30, FONT_MEDIUM, "No networks found.", COLOR_INK);
  } else if (shown < (int)wifi_.size()) {
    textAt_(g, UI_MARGIN, y + 22, FONT_SMALL,
            "+ " + String((int)wifi_.size() - shown) + " more (see log)",
            COLOR_INK);
  }

  paintFooter_(g, w, h);
}

// ---------------------------------------------------------------------------
//  Full-screen message
// ---------------------------------------------------------------------------
void DuetTheme::showMessage(const String& title, const String& line1,
                            const String& line2, const String& line3) {
  drv_.render([&](DisplayType& g) {
    const int w = g.width();
    paintHeader_(g, w);
    int y = UI_HEADER_H + 80;
    textAt_(g, UI_MARGIN, y, FONT_LARGE,
            fit_(g, FONT_LARGE, title, w - 2 * UI_MARGIN), COLOR_INK);
    y += 44;
    if (line1.length()) {
      textAt_(g, UI_MARGIN, y, FONT_MEDIUM,
              fit_(g, FONT_MEDIUM, line1, w - 2 * UI_MARGIN), COLOR_INK);
      y += 32;
    }
    if (line2.length()) {
      textAt_(g, UI_MARGIN, y, FONT_NORMAL,
              fit_(g, FONT_NORMAL, line2, w - 2 * UI_MARGIN), COLOR_INK);
      y += 30;
    }
    if (line3.length()) {
      textAt_(g, UI_MARGIN, y, FONT_NORMAL,
              fit_(g, FONT_NORMAL, line3, w - 2 * UI_MARGIN), COLOR_INK);
    }
  });
  drv_.hibernate();
}
