#include "HomeApp.h"

#include <LittleFS.h>

#include "AppManager.h"
#include "FilesApp.h"
#include "GamesApp.h"
#include "SettingsApp.h"
#include "Stats.h"
#include "WiFiApp.h"
#include "theme.h"

static const int kWindow = 3;  // covers shown at once

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
    for (File f = dd.openNextFile(); f && covers_.size() < 24; f = dd.openNextFile()) {
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

void HomeApp::onEnter() { scanBooks(); if (carousel_ > (int)covers_.size()) carousel_ = 0; }

void HomeApp::launch(int i) {
  switch (i) {
    case 0: nav->push(new GamesApp()); break;
    case 1: nav->push(new FilesApp()); break;   // Books & Images
    case 2: nav->push(new WiFiApp()); break;
    case 3: nav->push(new SettingsApp()); break;
    default: break;
  }
}

bool HomeApp::onButton(Btn b) {
  int maxStart = (int)covers_.size() - kWindow;
  if (maxStart < 0) maxStart = 0;
  switch (b) {
    case Btn::Left:  sel_ = (sel_ + kSections - 1) % kSections; return true;
    case Btn::Right: sel_ = (sel_ + 1) % kSections; return true;
    case Btn::Up:    if (carousel_ > 0) { --carousel_; return true; } return true;
    case Btn::Down:  if (carousel_ < maxStart) { ++carousel_; return true; } return true;
    case Btn::Confirm: launch(sel_); return true;
    default: return false;
  }
}

static void drawTick(GFXcanvas1& g, int cx, int cy) {
  g.fillCircle(cx, cy, 22, UI_BLACK);
  g.drawLine(cx - 10, cy, cx - 3, cy + 9, UI_WHITE);
  g.drawLine(cx - 9, cy, cx - 3, cy + 8, UI_WHITE);
  g.drawLine(cx - 3, cy + 9, cx + 12, cy - 9, UI_WHITE);
  g.drawLine(cx - 3, cy + 8, cx + 12, cy - 10, UI_WHITE);
}

void HomeApp::render(DuetDisplay& d) {
  auto& g = d.gfx();
  duet::headerBar(g, "Vix OS", "");

  // --- book carousel ("Continue reading") ---
  duet::text(g, UI_MARGIN, UI_HEADER_H + 26, FONT_MED, "Continue reading", UI_BLACK);
  if (covers_.size() > kWindow) {
    String pos = String(carousel_ + 1) + "-" +
                 String(min((int)covers_.size(), carousel_ + kWindow)) + " of " +
                 String((int)covers_.size());
    int w = duet::textWidth(g, FONT_BODY, pos);
    duet::text(g, SCREEN_W - UI_MARGIN - w, UI_HEADER_H + 26, FONT_BODY, pos, UI_BLACK);
  }
  int cy0 = UI_HEADER_H + 40;
  const int cw = 156, chh = 150, gap = 18;
  int startx = (SCREEN_W - (kWindow * cw + (kWindow - 1) * gap)) / 2;
  if (covers_.empty()) {
    g.drawRect(startx, cy0, cw, chh, UI_BLACK);
    duet::centerText(g, startx + cw / 2, cy0 + chh / 2, FONT_BODY, "No books yet", UI_BLACK);
  }
  for (int i = 0; i < kWindow; ++i) {
    int idx = carousel_ + i;
    if (idx >= (int)covers_.size()) break;
    int x = startx + i * (cw + gap);
    g.drawRect(x, cy0, cw, chh, UI_BLACK);
    duet::text(g, x + 10, cy0 + 30, FONT_BODY,
               duet::fit(g, FONT_BODY, covers_[idx], cw - 20), UI_BLACK);
    if (done_[idx]) drawTick(g, x + cw / 2, cy0 + chh / 2 + 10);
  }
  if (covers_.size() > kWindow)
    duet::text(g, UI_MARGIN, cy0 + chh + 22, FONT_BODY,
               "Side buttons: scroll books", UI_BLACK);

  // --- 2x2 section grid, stepped linearly with Left/Right ---
  const int m = 16;
  int gy = cy0 + chh + (covers_.size() > kWindow ? 44 : 24);
  int tw = (SCREEN_W - 3 * m) / 2;
  int th = (SCREEN_H - UI_FOOTER_H - gy - m) / 2 - m / 2;
  const char* titles[4] = {"Games", "Books", "Wi-Fi", "Settings"};
  const char* subs[4]   = {"Play offline", "Read & images", "Connect & transfer",
                           "Info & clock"};
  String counts[4] = {"12", "", (Net::connected() ? "on" : "off"),
                      String("v") + FW_VERSION};
  for (int i = 0; i < 4; ++i) {
    int col = i % 2, row = i / 2;
    int x = m + col * (tw + m);
    int y = gy + row * (th + m);
    duet::tile(g, x, y, tw, th, titles[i], subs[i], counts[i], i == sel_);
  }

  duet::buttonBar(g, "", "Open", "< Prev", "Next >");
}
