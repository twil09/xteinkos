#include "SettingsApp.h"

#include "AppManager.h"
#include "Battery.h"
#include "Clock.h"
#include "Net.h"
#include "QrApp.h"
#include "Stats.h"
#include "theme.h"

bool SettingsApp::onButton(Btn b) {
  switch (b) {
    case Btn::Up:   sel_ = (sel_ + kRows - 1) % kRows; return true;
    case Btn::Down: sel_ = (sel_ + 1) % kRows; return true;
    case Btn::Left:
      if (sel_ == 1) { Clock::setOffsetHours(Clock::offsetHours() - 1); return true; }
      sel_ = (sel_ + kRows - 1) % kRows; return true;
    case Btn::Right:
      if (sel_ == 1) { Clock::setOffsetHours(Clock::offsetHours() + 1); return true; }
      sel_ = (sel_ + 1) % kRows; return true;
    case Btn::Confirm:
      if (sel_ == 0) { nav->requestSleep(); return true; }
      if (sel_ == 2) { nav->push(new QrApp()); return true; }
      return true;  // time-zone row: adjust with Left/Right
    default: return false;  // Back -> home
  }
}

void SettingsApp::render(DuetDisplay& d) {
  auto& g = d.gfx();
  duet::headerBar(g, "Settings", "v" FW_VERSION);

  int y = UI_HEADER_H + 30;
  auto line = [&](const String& s) {
    duet::text(g, UI_MARGIN, y, FONT_BODY, s, UI_BLACK);
    y += 26;
  };

  duet::text(g, UI_MARGIN, y, FONT_MED, "Device", UI_BLACK); y += 28;
  line(String("Model: ") + ESP.getChipModel() + "  rev " + ESP.getChipRevision());
  line(String("Flash: ") + (ESP.getFlashChipSize() / (1024 * 1024)) + " MB" +
       "    Free RAM: " + (ESP.getFreeHeap() / 1024) + " KB");
  line(String("Battery: ") + Battery::percent() + "%   (" +
       Battery::millivolts() + " mV)");
  line(Net::connected() ? ("Wi-Fi: " + Net::ssid() + "  " + Net::ip())
                        : "Wi-Fi: off");
  String when = Clock::synced() ? (Clock::hhmm() + "   " + Clock::dateStr())
                                : "not set (connect Wi-Fi)";
  line(String("Time: ") + when);
  y += 10;

  duet::text(g, UI_MARGIN, y, FONT_MED, "Reading", UI_BLACK); y += 28;
  line(String("Time: ") + Stats::totalMinutes() + " min   Pages: " + Stats::totalPages());
  int sc = Stats::speedScore();
  line(String("Speed score: ") + (sc ? String(sc) + " / 100" : String("--")));
  y += 12;

  const int rowH = 54;
  int ry = SCREEN_H - UI_FOOTER_H - kRows * rowH - 8;
  int off = Clock::offsetHours();
  String tz = String("Time zone:  UTC") + (off >= 0 ? "+" : "") + off + "   (Left/Right)";
  duet::listRow(g, 0, ry, SCREEN_W, rowH, "Sleep now", sel_ == 0);
  duet::listRow(g, 0, ry + rowH, SCREEN_W, rowH, tz, sel_ == 1);
  duet::listRow(g, 0, ry + 2 * rowH, SCREEN_W, rowH, "QR codes", sel_ == 2);

  duet::buttonBar(g, "Back", "Select", "Up", "Down");
}
