#include "LibraryApp.h"

#include <LittleFS.h>
#include <SDCardManager.h>

#include "AppManager.h"
#include "ReaderApp.h"
#include "theme.h"

static bool isBook(const String& n) {
  String l = n;
  l.toLowerCase();
  return l.endsWith(".txt") || l.endsWith(".md");
}

static String baseName(const String& n) {
  int s = n.lastIndexOf('/');
  return (s >= 0) ? n.substring(s + 1) : n;
}

void LibraryApp::addDir(bool sd, const char* dir) {
  if (sd) {
    if (!SdMan.ready()) return;
    std::vector<String> files = SdMan.listFiles(dir, 200);
    for (auto& f : files) {
      String base = baseName(f);
      if (!isBook(base)) continue;
      String path = f.startsWith("/") ? f
                    : (String(dir) + (String(dir).endsWith("/") ? "" : "/") + base);
      paths_.push_back(path);
      names_.push_back("[SD] " + base);
      fromSd_.push_back(1);
    }
  } else {
    File dd = LittleFS.open(dir);
    if (!dd || !dd.isDirectory()) return;
    for (File f = dd.openNextFile(); f; f = dd.openNextFile()) {
      if (f.isDirectory()) continue;
      String base = baseName(String(f.name()));
      if (!isBook(base)) continue;
      paths_.push_back(String("/books/") + base);
      names_.push_back(base);
      fromSd_.push_back(0);
    }
  }
}

void LibraryApp::scan() {
  paths_.clear();
  names_.clear();
  fromSd_.clear();
  sel_ = 0;
  scrollTop_ = 0;

  addDir(false, "/books");   // internal flash
  SdMan.begin();             // (re)mount SD if a card is present
  addDir(true, "/");         // SD root
  addDir(true, "/books");    // SD /books
}

bool LibraryApp::onButton(Btn b) {
  if (paths_.empty()) return false;  // Back -> home
  int n = (int)paths_.size();
  switch (b) {
    case Btn::Up:   sel_ = (sel_ - 1 + n) % n; return true;
    case Btn::Down: sel_ = (sel_ + 1) % n; return true;
    case Btn::Confirm:
      nav->push(new ReaderApp(paths_[sel_], fromSd_[sel_] != 0));
      return true;
    default: return false;  // Back -> home
  }
}

void LibraryApp::render(DuetDisplay& d) {
  auto& g = d.gfx();
  duet::header(g, "Library");

  if (paths_.empty()) {
    duet::centerText(g, SCREEN_W / 2, SCREEN_H / 2 - 10, FONT_MED,
                     "No books found", UI_BLACK);
    duet::centerText(g, SCREEN_W / 2, SCREEN_H / 2 + 20, FONT_BODY,
                     "Add .txt/.md to a microSD, or to data/books + uploadfs",
                     UI_BLACK);
    duet::footerHint(g, "Back: home");
    return;
  }

  const int rowH = 60;
  const int rows = (SCREEN_H - UI_HEADER_H - UI_FOOTER_H) / rowH;
  if (sel_ < scrollTop_) scrollTop_ = sel_;
  if (sel_ >= scrollTop_ + rows) scrollTop_ = sel_ - rows + 1;
  if (scrollTop_ < 0) scrollTop_ = 0;

  int y = UI_HEADER_H;
  for (int i = 0; i < rows; ++i) {
    int idx = scrollTop_ + i;
    if (idx >= (int)names_.size()) break;
    duet::listRow(g, 0, y, SCREEN_W, rowH, names_[idx], idx == sel_);
    y += rowH;
  }
  duet::footerHint(g, "Up/Down: select   Confirm: read   Back: home");
}
