#include "HomeApp.h"

#include <LittleFS.h>
#include <WiFi.h>

#include "AppManager.h"
#include "FilesApp.h"
#include "GamesApp.h"
#include "SettingsApp.h"
#include "Stats.h"
#include "WiFiApp.h"
#include "theme.h"

static bool isBook(const String& n) {
  String l = n; l.toLowerCase();
  const char* e[] = {".txt", ".md", ".xtc", ".html", ".htm", ".rtf",
                     ".epub", ".mobi", ".pdf", ".cbz", ".cbr"};
  for (auto x : e) if (l.endsWith(x)) return true;
  return false;
}

void HomeApp::scanBooks() {
  covers_.clear();
  done_.clear();
  File dd = LittleFS.open("/books");
  if (dd && dd.isDirectory()) {
    for (File f = dd.openNextFile(); f && covers_.size() < 3; f = dd.openNextFile()) {
      if (f.isDirectory()) continue;
      String name = f.name();
      int s = name.lastIndexOf('/');
      String base = (s >= 0) ? name.substring(s + 1) : name;
      if (!isBook(base)) continue;
      covers_.push_back(base);
      done_.push_back(Stats::isCompleted("/books/" + base) ? 1 : 0);
    }
  }
}

void HomeApp::onEnter() { scanBooks(); }

void HomeApp::launch(int i) {
  switch (i) {
    case 0: nav->push(new GamesApp()); break;
    case 1: nav->push(new FilesApp()); break;
    case 2: nav->push(new WiFiApp()); break;
    case 3: nav->push(new SettingsApp()); break;
    default: break;
  }
}

bool HomeApp::onButton(Btn b) {
  int row = sel_ / 2, col = sel_ % 2;
  switch (b) {
    case Btn::Left:  if (col == 1) --sel_; return true;
    case Btn::Right: if (col == 0) ++sel_; return true;
    case Btn::Up:    if (row == 1) sel_ -= 2; return true;
    case Btn::Down:  if (row == 0) sel_ += 2; return true;
    case Btn::Confirm: launch(sel_); return true;
    default: return false;
  }
}

static void drawTick(GFXcanvas1& g, int cx, int cy) {
  // filled disc + white check
  g.fillCircle(cx, cy, 22, UI_BLACK);
  g.drawLine(cx - 10, cy, cx - 3, cy + 9, UI_WHITE);
  g.drawLine(cx - 9, cy, cx - 3, cy + 8, UI_WHITE);
  g.drawLine(cx - 3, cy + 9, cx + 12, cy - 9, UI_WHITE);
  g.drawLine(cx - 3, cy + 8, cx + 12, cy - 10, UI_WHITE);
}

void HomeApp::render(DuetDisplay& d) {
  auto& g = d.gfx();
  String wifi = (WiFi.status() == WL_CONNECTED) ? WiFi.SSID() : String("Wi-Fi off");
  duet::headerBar(g, "Vix OS", wifi);

  // --- book carousel ---
  duet::text(g, UI_MARGIN, UI_HEADER_H + 24, FONT_MED, "Continue reading", UI_BLACK);
  int cy0 = UI_HEADER_H + 36;
  const int cw = 156, chh = 150, gap = 18;
  int startx = (SCREEN_W - (3 * cw + 2 * gap)) / 2;
  if (covers_.empty()) {
    g.drawRect(startx, cy0, cw, chh, UI_BLACK);
    duet::centerText(g, startx + cw / 2, cy0 + chh / 2, FONT_BODY, "No books", UI_BLACK);
  }
  for (size_t i = 0; i < covers_.size(); ++i) {
    int x = startx + (int)i * (cw + gap);
    g.drawRect(x, cy0, cw, chh, UI_BLACK);
    // pseudo-cover: title wrapped
    String t = covers_[i];
    duet::text(g, x + 10, cy0 + 30, FONT_BODY,
               duet::fit(g, FONT_BODY, t, cw - 20), UI_BLACK);
    if (done_[i]) drawTick(g, x + cw / 2, cy0 + chh / 2 + 10);
  }

  // --- 2x2 section grid ---
  const int m = 16;
  int gy = cy0 + chh + 22;
  int tw = (SCREEN_W - 3 * m) / 2;
  int th = (SCREEN_H - UI_FOOTER_H - gy - m) / 2 - m / 2;
  const char* titles[4] = {"Games", "Files", "Wi-Fi", "Settings"};
  const char* subs[4]   = {"Play", "Books & images", "Connect", "Info & stats"};
  String counts[4] = {"12 apps", "", (WiFi.status() == WL_CONNECTED ? "on" : "off"),
                      String("v") + FW_VERSION};
  for (int i = 0; i < 4; ++i) {
    int col = i % 2, row = i / 2;
    int x = m + col * (tw + m);
    int y = gy + row * (th + m);
    duet::tile(g, x, y, tw, th, titles[i], subs[i], counts[i], i == sel_);
  }

  duet::buttonBar(g, "", "Open", "<", ">");
}
