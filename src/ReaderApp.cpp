#include "ReaderApp.h"

#include <ctype.h>

#include "Epub.h"
#include "ProgressStore.h"
#include "ReaderSettings.h"
#include "Stats.h"
#include "theme.h"

#define READER_TOP    (UI_HEADER_H + 16)
#define READER_DWELL_CAP 180000UL  // cap per-page dwell at 3 min (idle guard)

ReaderApp::ReaderApp(const String& path, bool fromSd)
    : path_(path), fromSd_(fromSd) {
  int slash = path_.lastIndexOf('/');
  title_ = (slash >= 0) ? path_.substring(slash + 1) : path_;
  key_ = (fromSd_ ? String("sd:") : String("")) + path_;
  String l = path_; l.toLowerCase();
  if (l.endsWith(".epub")) { epub_ = true; fmt_ = 0; }  // extracted to plain text
  else if (l.endsWith(".html") || l.endsWith(".htm")) fmt_ = 1;
  else if (l.endsWith(".rtf")) fmt_ = 2;
  else fmt_ = 0;  // txt / md / xtc treated as plain
}

ReaderApp::~ReaderApp() { flush(0); }

void ReaderApp::onEnter() {
  if (epub_) {
    Epub::Result res = Epub::prepare(path_, fromSd_);
    if (!res.ok) { ok_ = false; err_ = res.error; pageStartMs_ = millis(); return; }
    if (res.title.length()) title_ = res.title;
    ok_ = src_.openLittleFS(res.textPath);  // read the extracted plain text
  } else {
    ok_ = fromSd_ ? src_.openSD(path_) : src_.openLittleFS(path_);
  }
  if (ok_) {
    size_ = src_.size();
    uint32_t saved = ProgressStore::load(key_);
    if (saved >= size_) saved = 0;
    starts_.clear();
    starts_.push_back(saved);
    idx_ = 0;
  }
  pageStartMs_ = millis();
}

void ReaderApp::flush(int pagesDelta) {
  uint32_t now = millis();
  uint32_t dwell = now - pageStartMs_;
  if (dwell > READER_DWELL_CAP) dwell = READER_DWELL_CAP;
  Stats::addSession(dwell, pagesDelta);
  pageStartMs_ = now;
}

void ReaderApp::layout(DuetDisplay& d, uint32_t start, std::vector<String>& lines) {
  lines.clear();
  firstLine_ = "";
  static char buf[4096];
  if (!ok_) { nextOffset_ = start; return; }
  const GFXfont* font = ReaderSettings::font();
  const int textW = SCREEN_W - 2 * ReaderSettings::margin();
  const int lineH = ReaderSettings::lineHeight();
  const int maxLines = (SCREEN_H - UI_FOOTER_H - READER_TOP) / lineH;

  src_.seek(start);
  size_t got = src_.read(buf, sizeof(buf) - 1);
  buf[got] = 0;

  auto& g = d.gfx();
  size_t i = 0;
  int lineCount = 0;

  auto nextChar = [&](void) -> int {  // display char, ' ', '\n', or -1 (skip)
    char c = buf[i];
    if (fmt_ == 1) {              // HTML: drop tags, keep text
      if (c == '<') {
        size_t tagStart = i + 1;
        while (i < got && buf[i] != '>') ++i;
        if (i < got) ++i;
        char t = (tagStart < got) ? buf[tagStart] : 0;
        if (t == 'p' || t == 'P' || t == 'b' || t == 'B' || t == 'd' || t == 'D')
          return '\n';
        return ' ';
      }
    } else if (fmt_ == 2) {       // RTF: drop control words and braces
      if (c == '{' || c == '}') { ++i; return -1; }
      if (c == '\\') {
        ++i;
        if (i < got && buf[i] == 'p') return (++i, '\n');
        while (i < got && isalpha((unsigned char)buf[i])) ++i;
        if (i < got && buf[i] == ' ') ++i;
        return ' ';
      }
    }
    ++i;
    return (unsigned char)c;
  };

  while (lineCount < maxLines && i < got) {
    String line = "";
    while (true) {
      if (i >= got) break;
      int ch = nextChar();
      if (ch < 0) continue;
      if (ch == '\r') continue;
      if (ch == '\n') break;
      if (ch == ' ') {
        if (line.length() && line[line.length() - 1] != ' ') line += ' ';
        continue;
      }
      String cand = line + (char)ch;
      if (duet::textWidth(g, font, cand) <= textW) {
        line = cand;
      } else {
        --i;
        break;
      }
    }
    while (line.length() && line[line.length() - 1] == ' ') line.remove(line.length() - 1);
    if (firstLine_.length() == 0 && line.length()) firstLine_ = line;
    lines.push_back(line);
    ++lineCount;
  }
  nextOffset_ = start + i;
  if (nextOffset_ > size_) nextOffset_ = size_;
}

void ReaderApp::saveProgress() {
  if (ok_ && idx_ >= 0 && idx_ < (int)starts_.size())
    ProgressStore::save(key_, starts_[idx_]);
}

void ReaderApp::relayoutHere() {
  uint32_t cur = (idx_ >= 0 && idx_ < (int)starts_.size()) ? starts_[idx_] : 0;
  starts_.clear();
  starts_.push_back(cur);
  idx_ = 0;
}

void ReaderApp::jumpTo(uint32_t offset) {
  if (offset > size_) offset = size_;
  starts_.clear();
  starts_.push_back(offset);
  idx_ = 0;
  flush(0);
  saveProgress();
}

bool ReaderApp::pageForward() {
  if (nextOffset_ < size_) {
    if (idx_ == (int)starts_.size() - 1) starts_.push_back(nextOffset_);
    ++idx_;
    flush(1);
    saveProgress();
  }
  return true;
}

bool ReaderApp::pageBack() {
  if (idx_ > 0) { --idx_; flush(0); saveProgress(); }
  return true;
}

// ---- input dispatch --------------------------------------------------------

bool ReaderApp::onButton(Btn b) {
  switch (rmode_) {
    case RMode::Reading:   return onButtonReading(b);
    case RMode::Menu:      return onButtonMenu(b);
    case RMode::Fonts:     return onButtonFonts(b);
    case RMode::Bookmarks: return onButtonBookmarks(b);
  }
  return false;
}

bool ReaderApp::onButtonReading(Btn b) {
  switch (b) {
    case Btn::Down:
    case Btn::Right: return pageForward();
    case Btn::Up:
    case Btn::Left:  return pageBack();
    case Btn::Confirm: rmode_ = RMode::Menu; menuSel_ = 0; return true;
    default: return false;  // Back -> library
  }
}

bool ReaderApp::onButtonMenu(Btn b) {
  const int rows = 4;
  switch (b) {
    case Btn::Up:   menuSel_ = (menuSel_ + rows - 1) % rows; return true;
    case Btn::Down: menuSel_ = (menuSel_ + 1) % rows; return true;
    case Btn::Confirm:
      if (menuSel_ == 0) { rmode_ = RMode::Fonts; fontSel_ = 0; return true; }
      if (menuSel_ == 1) {  // add / remove bookmark at current page
        uint32_t cur = starts_[idx_];
        if (BookmarkStore::hasAt(key_, cur)) {
          auto v = BookmarkStore::list(key_);
          for (int i = 0; i < (int)v.size(); ++i)
            if (labs((long)v[i].offset - (long)cur) < 200) { BookmarkStore::removeAt(key_, i); break; }
        } else {
          String sum = firstLine_;
          if (sum.length() > 42) sum = sum.substring(0, 42);
          BookmarkStore::add(key_, cur, sum);
        }
        rmode_ = RMode::Reading;
        return true;
      }
      if (menuSel_ == 2) {
        bms_ = BookmarkStore::list(key_);
        bmSel_ = 0;
        rmode_ = RMode::Bookmarks;
        return true;
      }
      rmode_ = RMode::Reading;  // Close
      return true;
    case Btn::Back: rmode_ = RMode::Reading; return true;
    default: return true;  // swallow (stay in menu)
  }
}

bool ReaderApp::onButtonFonts(Btn b) {
  const int rows = 4;
  switch (b) {
    case Btn::Up:   fontSel_ = (fontSel_ + rows - 1) % rows; return true;
    case Btn::Down: fontSel_ = (fontSel_ + 1) % rows; return true;
    case Btn::Left:
    case Btn::Right: {
      int d = (b == Btn::Right) ? 1 : -1;
      if (fontSel_ == 0) ReaderSettings::setFamily(ReaderSettings::family() + d);
      else if (fontSel_ == 1) ReaderSettings::setSizeIdx(ReaderSettings::sizeIdx() + d);
      else if (fontSel_ == 2) ReaderSettings::setSpacingIdx(ReaderSettings::spacingIdx() + d);
      else ReaderSettings::setMarginIdx(ReaderSettings::marginIdx() + d);
      relayoutHere();
      return true;
    }
    case Btn::Confirm: rmode_ = RMode::Reading; return true;
    case Btn::Back: rmode_ = RMode::Menu; return true;
    default: return true;
  }
}

bool ReaderApp::onButtonBookmarks(Btn b) {
  int n = (int)bms_.size();
  switch (b) {
    case Btn::Up:   if (n) bmSel_ = (bmSel_ + n - 1) % n; return true;
    case Btn::Down: if (n) bmSel_ = (bmSel_ + 1) % n; return true;
    case Btn::Left:  // delete selected
      if (n) { BookmarkStore::removeAt(key_, bmSel_); bms_ = BookmarkStore::list(key_);
               if (bmSel_ >= (int)bms_.size()) bmSel_ = bms_.empty() ? 0 : bms_.size() - 1; }
      return true;
    case Btn::Confirm:
      if (n) { jumpTo(bms_[bmSel_].offset); rmode_ = RMode::Reading; }
      return true;
    case Btn::Back: rmode_ = RMode::Menu; return true;
    default: return true;
  }
}

// ---- rendering -------------------------------------------------------------

void ReaderApp::render(DuetDisplay& d) {
  switch (rmode_) {
    case RMode::Reading:   renderReading(d); break;
    case RMode::Menu:      renderMenu(d); break;
    case RMode::Fonts:     renderFonts(d); break;
    case RMode::Bookmarks: renderBookmarks(d); break;
  }
}

void ReaderApp::renderReading(DuetDisplay& d) {
  auto& g = d.gfx();
  int pct = size_ ? (int)((uint64_t)starts_[idx_] * 100 / size_) : 0;
  duet::headerBar(g, title_, String(pct) + "%");

  if (!ok_) {
    duet::centerText(g, SCREEN_W / 2, SCREEN_H / 2 - 12, FONT_MED,
                     "Could not open book.", UI_BLACK);
    if (err_.length())
      duet::centerText(g, SCREEN_W / 2, SCREEN_H / 2 + 16, FONT_BODY, err_, UI_BLACK);
    duet::buttonBar(g, "Back", "", "", "");
    return;
  }

  std::vector<String> lines;
  layout(d, starts_[idx_], lines);
  const GFXfont* font = ReaderSettings::font();
  const int m = ReaderSettings::margin();
  const int lineH = ReaderSettings::lineHeight();
  int y = READER_TOP + lineH - 6;
  for (auto& ln : lines) {
    if (ln.length()) duet::text(g, m, y, font, ln, UI_BLACK);
    y += lineH;
  }
  if (BookmarkStore::hasAt(key_, starts_[idx_]))  // corner mark for a bookmarked page
    g.fillTriangle(SCREEN_W - 22, UI_HEADER_H, SCREEN_W - 2, UI_HEADER_H,
                   SCREEN_W - 2, UI_HEADER_H + 20, UI_BLACK);
  if (nextOffset_ >= size_ && size_ > 0) Stats::setCompleted(key_, true);

  duet::buttonBar(g, "Back", "Menu", "Prev", "Next");
}

void ReaderApp::renderMenu(DuetDisplay& d) {
  auto& g = d.gfx();
  duet::headerBar(g, "Reading menu", "");
  uint32_t cur = starts_[idx_];
  bool marked = BookmarkStore::hasAt(key_, cur);
  int n = (int)BookmarkStore::list(key_).size();
  const int rowH = 74;
  int y = UI_HEADER_H + 20;
  String f = String("Fonts & layout   (") + ReaderSettings::familyName() + " " +
             ReaderSettings::sizeName() + ")";
  duet::listRow(g, 0, y, SCREEN_W, rowH, f, menuSel_ == 0);
  duet::listRow(g, 0, y + rowH, SCREEN_W, rowH,
                marked ? "Remove bookmark here" : "Add bookmark here", menuSel_ == 1);
  duet::listRow(g, 0, y + 2 * rowH, SCREEN_W, rowH,
                String("Bookmarks  (") + n + ")", menuSel_ == 2);
  duet::listRow(g, 0, y + 3 * rowH, SCREEN_W, rowH, "Close", menuSel_ == 3);
  duet::buttonBar(g, "Close", "Select", "Up", "Down");
}

void ReaderApp::renderFonts(DuetDisplay& d) {
  auto& g = d.gfx();
  duet::headerBar(g, "Fonts & layout", "");
  const int rowH = 66;
  int y = UI_HEADER_H + 24;
  auto row = [&](int i, const String& label, const String& val) {
    duet::listRow(g, 0, y, SCREEN_W, rowH, label + ":   < " + val + " >", fontSel_ == i);
    y += rowH;
  };
  row(0, "Font", ReaderSettings::familyName());
  row(1, "Text size", ReaderSettings::sizeName());
  row(2, "Line spacing", ReaderSettings::spacingName());
  row(3, "Margins", ReaderSettings::marginName());

  // live sample
  duet::text(g, ReaderSettings::margin(), y + 30, ReaderSettings::font(),
             "The quick brown fox.", UI_BLACK);
  duet::buttonBar(g, "Back", "Done", "<  ", "  >");
}

void ReaderApp::renderBookmarks(DuetDisplay& d) {
  auto& g = d.gfx();
  duet::headerBar(g, "Bookmarks", "");
  if (bms_.empty()) {
    duet::centerText(g, SCREEN_W / 2, SCREEN_H / 2, FONT_MED,
                     "No bookmarks yet.", UI_BLACK);
    duet::buttonBar(g, "Back", "", "", "");
    return;
  }
  const int rowH = 58;
  int y = UI_HEADER_H + 16;
  int perPage = (SCREEN_H - UI_FOOTER_H - y) / rowH;
  int first = 0;
  if (bmSel_ >= perPage) first = bmSel_ - perPage + 1;
  for (int i = first; i < (int)bms_.size() && i < first + perPage; ++i) {
    int pct = size_ ? (int)((uint64_t)bms_[i].offset * 100 / size_) : 0;
    String s = String(pct) + "%  " + bms_[i].summary;
    duet::listRow(g, 0, y, SCREEN_W, rowH, s, i == bmSel_);
    y += rowH;
  }
  duet::buttonBar(g, "Back", "Go", "Delete", "Down");
}
