#include "LibraryApp.h"

#include <LittleFS.h>
#include <SDCardManager.h>

#include "AppManager.h"
#include "PlaceholderApp.h"
#include "ReaderApp.h"
#include "theme.h"

// 0 = text (render now), 1 = ebook staged, 2 = image, -1 = ignore.
static int classify(const String& name) {
  String l = name; l.toLowerCase();
  const char* text[]  = {".txt", ".md", ".xtc", ".html", ".htm", ".rtf"};
  const char* ebook[] = {".epub", ".mobi", ".pdf", ".cbz", ".cbr"};
  const char* image[] = {".jpg", ".jpeg", ".png", ".bmp", ".gif"};
  for (auto e : text)  if (l.endsWith(e)) return 0;
  for (auto e : ebook) if (l.endsWith(e)) return 1;
  for (auto e : image) if (l.endsWith(e)) return 2;
  return -1;
}

static String baseName(const String& n) {
  int s = n.lastIndexOf('/');
  return (s >= 0) ? n.substring(s + 1) : n;
}

void LibraryApp::addDir(bool sd, const char* dir) {
  auto wanted = [&](int cls) {
    if (cls < 0) return false;
    return cat_ == Cat::Images ? (cls == 2) : (cls == 0 || cls == 1);
  };
  if (sd) {
    if (!SdMan.ready()) return;
    std::vector<String> files = SdMan.listFiles(dir, 300);
    for (auto& f : files) {
      String base = baseName(f);
      if (!wanted(classify(base))) continue;
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
      if (!wanted(classify(base))) continue;
      paths_.push_back(String(dir) + "/" + base);
      names_.push_back(base);
      fromSd_.push_back(0);
    }
  }
}

void LibraryApp::scan() {
  paths_.clear(); names_.clear(); fromSd_.clear();
  sel_ = 0; scrollTop_ = 0;
  const char* flashDir = (cat_ == Cat::Images) ? "/images" : "/books";
  addDir(false, flashDir);
  SdMan.begin();
  addDir(true, "/");
  addDir(true, (cat_ == Cat::Images) ? "/images" : "/books");
}

void LibraryApp::openSelected() {
  const String& path = paths_[sel_];
  bool sd = fromSd_[sel_] != 0;
  int cls = classify(baseName(path));
  String low = baseName(path); low.toLowerCase();
  if (cls == 0 || low.endsWith(".epub")) {
    nav->push(new ReaderApp(path, sd));       // .epub extracts to text on open
  } else if (cls == 1) {
    nav->push(new PlaceholderApp("Format staged",
                                 baseName(path) + " - viewer coming soon"));
  } else if (cls == 2) {
    nav->push(new PlaceholderApp("Image viewer",
                                 baseName(path) + " - decoding coming soon"));
  }
}

bool LibraryApp::onButton(Btn b) {
  if (paths_.empty()) return false;  // Back -> home
  int n = (int)paths_.size();
  switch (b) {
    case Btn::Up:
    case Btn::Left:    sel_ = (sel_ - 1 + n) % n; return true;
    case Btn::Down:
    case Btn::Right:   sel_ = (sel_ + 1) % n; return true;
    case Btn::Confirm: openSelected(); return true;
    default: return false;  // Back -> home
  }
}

void LibraryApp::render(DuetDisplay& d) {
  auto& g = d.gfx();
  const char* title = (cat_ == Cat::Images) ? "Images" : "Books";
  duet::headerBar(g, title, String((int)paths_.size()));

  if (paths_.empty()) {
    duet::centerText(g, SCREEN_W / 2, SCREEN_H / 2 - 10, FONT_MED,
                     "Nothing here yet", UI_BLACK);
    duet::centerText(g, SCREEN_W / 2, SCREEN_H / 2 + 18, FONT_BODY,
                     cat_ == Cat::Images ? "Add images to microSD or /images"
                                         : "Add .txt/.epub to microSD or /books",
                     UI_BLACK);
    duet::buttonBar(g, "Back", "", "", "");
    return;
  }

  const int rowH = 58;
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
  duet::buttonBar(g, "Back", "Open", "Up", "Down");
}
