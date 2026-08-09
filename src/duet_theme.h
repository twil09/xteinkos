// ============================================================================
//  duet_theme.h — Duet-styled rendering for the X3 e-ink display
// ----------------------------------------------------------------------------
//  Compose a frame by calling begin(), then the set/add methods, then commit().
//  On the black/white panel the Duet palette collapses to ink-on-white: the
//  header is a solid black bar with reversed text, cards are outlined, and
//  status badges are distinguished by fill (known = solid, unknown = outline).
// ============================================================================
#pragma once

#include <Arduino.h>
#include <vector>

#include "config.h"
#include "display_driver.h"
#include "qr_display.h"
#include "wifi_scanner.h"

class DuetTheme {
 public:
  explicit DuetTheme(DisplayDriver& drv) : drv_(drv) {}

  // ---- frame composition ----
  void begin();                                    // reset state for a frame
  void setHeader(const String& title);
  void addWiFi(const ScannedAP& ap);
  void setGeoIP(const String& ip, const String& country, const String& city);
  void setStatus(const String& status, bool isError);
  void setQR(const String& url, const String& label);
  void commit();                                   // paint to the panel

  // ---- one-shot full-screen message (boot / connecting / errors) ----
  void showMessage(const String& title, const String& line1 = "",
                   const String& line2 = "", const String& line3 = "");

  // ---- spec-compatible aliases ----
  void renderHeader(const char* title) { setHeader(title); }
  void renderWiFiResults(const std::vector<ScannedAP>& nets);
  void renderStatusBar(const char* status, bool isError) {
    setStatus(status, isError);
  }
  void renderQRCode(const char* url, const char* label) { setQR(url, label); }
  void clear() { begin(); }
  void update() { commit(); }

 private:
  DisplayDriver& drv_;

  String                 title_ = FW_NAME;
  std::vector<ScannedAP> wifi_;
  String                 geoIp_, geoCountry_, geoCity_;
  bool                   hasGeo_ = false;
  String                 status_;
  bool                   statusErr_ = false;
  String                 qrUrl_, qrLabel_;
  bool                   hasQr_ = false;
  QRDisplay              qr_;

  void paint_(DisplayType& g);
  void paintHeader_(DisplayType& g, int w);
  int  paintGeo_(DisplayType& g, int w, int y);
  void paintWiFiCard_(DisplayType& g, int w, int y, const ScannedAP& ap);
  void paintFooter_(DisplayType& g, int w, int h);

  void drawBadge_(DisplayType& g, int x, int y, int w, int h, const char* text,
                  bool filled);
  static void textAt_(DisplayType& g, int x, int baselineY, const GFXfont* font,
                      const String& s, uint16_t color);
  static String fit_(DisplayType& g, const GFXfont* font, const String& s,
                     int maxW);
};
