#include "ReaderApp.h"

#include "ProgressStore.h"
#include "theme.h"

#define READER_TOP    (UI_HEADER_H + 28)
#define READER_LINE_H 24
#define READER_LINES  ((SCREEN_H - UI_FOOTER_H - READER_TOP) / READER_LINE_H)
#define READER_TEXT_W (SCREEN_W - 2 * UI_MARGIN)

ReaderApp::ReaderApp(const String& path, bool fromSd)
    : path_(path), fromSd_(fromSd) {
  int slash = path_.lastIndexOf('/');
  title_ = (slash >= 0) ? path_.substring(slash + 1) : path_;
  key_ = (fromSd_ ? String("sd:") : String("")) + path_;
}

void ReaderApp::onEnter() {
  ok_ = fromSd_ ? src_.openSD(path_) : src_.openLittleFS(path_);
  if (ok_) {
    size_ = src_.size();
    uint32_t saved = ProgressStore::load(key_);
    if (saved >= size_) saved = 0;
    starts_.clear();
    starts_.push_back(saved);
    idx_ = 0;
  }
}

void ReaderApp::layout(DuetDisplay& d, uint32_t start, std::vector<String>& lines) {
  lines.clear();
  static char buf[4096];
  if (!ok_) { nextOffset_ = start; return; }
  src_.seek(start);
  size_t got = src_.read(buf, sizeof(buf) - 1);
  buf[got] = 0;

  auto& g = d.gfx();
  size_t i = 0;
  int lineCount = 0;
  while (lineCount < READER_LINES && i < got) {
    String line = "";
    while (true) {
      if (i >= got) break;
      char c = buf[i];
      if (c == '\r') { ++i; continue; }
      if (c == '\n') { ++i; break; }  // paragraph / hard break
      size_t wordStart = i;
      while (i < got && buf[i] != ' ' && buf[i] != '\n' && buf[i] != '\r') ++i;
      String word;
      for (size_t k = wordStart; k < i; ++k) word += buf[k];
      while (i < got && buf[i] == ' ') ++i;  // consume trailing spaces
      String cand = line.length() ? line + " " + word : word;
      if (line.length() == 0 || duet::textWidth(g, FONT_BODY, cand) <= READER_TEXT_W) {
        line = cand;
      } else {
        i = wordStart;  // unread the overflowing word for the next line
        break;
      }
    }
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

bool ReaderApp::onButton(Btn b) {
  switch (b) {
    case Btn::Right:
    case Btn::Down:
      if (nextOffset_ < size_) {
        if (idx_ == (int)starts_.size() - 1) starts_.push_back(nextOffset_);
        ++idx_;
        saveProgress();
        return true;
      }
      return true;
    case Btn::Left:
    case Btn::Up:
      if (idx_ > 0) { --idx_; saveProgress(); return true; }
      return true;
    default:
      return false;  // Back -> library
  }
}

void ReaderApp::render(DuetDisplay& d) {
  auto& g = d.gfx();
  int pct = size_ ? (int)((uint64_t)starts_[idx_] * 100 / size_) : 0;
  duet::header(g, title_ + "    " + String(pct) + "%");

  if (!ok_) {
    duet::centerText(g, SCREEN_W / 2, SCREEN_H / 2, FONT_MED,
                     "Could not open file.", UI_BLACK);
    duet::footerHint(g, "Back: library");
    return;
  }

  std::vector<String> lines;
  layout(d, starts_[idx_], lines);
  int y = READER_TOP;
  for (auto& ln : lines) {
    if (ln.length()) duet::text(g, UI_MARGIN, y, FONT_BODY, ln, UI_BLACK);
    y += READER_LINE_H;
  }
  duet::footerHint(g, "Up/Left: prev   Down/Right: next   Back: library");
}
