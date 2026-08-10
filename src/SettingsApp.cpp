#include "SettingsApp.h"

#include <WiFi.h>

#include "AppManager.h"
#include "PlaceholderApp.h"
#include "QrApp.h"
#include "Stats.h"
#include "theme.h"

bool SettingsApp::onButton(Btn b) {
  switch (b) {
    case Btn::Up:
    case Btn::Left:    sel_ = (sel_ + 2) % 3; return true;
    case Btn::Down:
    case Btn::Right:   sel_ = (sel_ + 1) % 3; return true;
    case Btn::Confirm:
      if (sel_ == 0) { nav->requestSleep(); return true; }
      if (sel_ == 2) { nav->push(new QrApp()); return true; }
      nav->push(new PlaceholderApp("Share stats",
                                   "Send to another Vix OS device (coming)"));
      return true;
    default: return false;  // Back -> home
  }
}

void SettingsApp::render(DuetDisplay& d) {
  auto& g = d.gfx();
  duet::headerBar(g, "Settings", "v" FW_VERSION);

  int y = UI_HEADER_H + 30;
  auto line = [&](const String& s) {
    duet::text(g, UI_MARGIN, y, FONT_BODY, s, UI_BLACK);
    y += 28;
  };

  duet::text(g, UI_MARGIN, y, FONT_MED, "Device", UI_BLACK); y += 30;
  line(String("Model: ") + ESP.getChipModel() + "  rev " + ESP.getChipRevision());
  line(String("Flash: ") + (ESP.getFlashChipSize() / (1024 * 1024)) + " MB");
  line(String("Free RAM: ") + (ESP.getFreeHeap() / 1024) + " KB");
  line(WiFi.status() == WL_CONNECTED
           ? ("Wi-Fi: " + WiFi.SSID() + "  " + WiFi.localIP().toString())
           : "Wi-Fi: off");
  y += 12;

  duet::text(g, UI_MARGIN, y, FONT_MED, "Reading", UI_BLACK); y += 30;
  line(String("Time: ") + Stats::totalMinutes() + " min   Pages: " + Stats::totalPages());
  int sc = Stats::speedScore();
  line(String("Speed score: ") + (sc ? String(sc) + " / 100" : String("--")));
  y += 16;

  const int rowH = 56;
  int ry = SCREEN_H - UI_FOOTER_H - 3 * rowH - 10;
  duet::listRow(g, 0, ry, SCREEN_W, rowH, "Sleep now", sel_ == 0);
  duet::listRow(g, 0, ry + rowH, SCREEN_W, rowH, "Share reading stats", sel_ == 1);
  duet::listRow(g, 0, ry + 2 * rowH, SCREEN_W, rowH, "QR codes", sel_ == 2);

  duet::buttonBar(g, "Back", "Select", "Up", "Down");
}
