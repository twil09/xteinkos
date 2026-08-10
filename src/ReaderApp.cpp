#include "ReaderApp.h"

#include <ctype.h>

#include "ProgressStore.h"
#include "Stats.h"
#include "theme.h"

#define READER_TOP    (UI_HEADER_H + 20)
#define READER_LINE_H 26
#define READER_LINES  ((SCREEN_H - UI_FOOTER_H - READER_TOP) / READER_LINE_H)
#define READER_TEXT_W (SCREEN_W - 2 * UI_MARGIN)
#define READER_DWELL_CAP 180000UL  // cap per-page dwell at 3 min (idle guard)

ReaderApp::ReaderApp(const String& path, bool fromSd)
    : path_(path), fromSd_(fromSd) {
  int slash = path_.lastIndexOf('/');
  title_ = (slash >= 0) ? path_.substring(slash + 1) : path_;
  key_ = (fromSd_ ? String("sd:") : String("")) + path_;
  String l = path_; l.toLowerCase();
  if (l.endsWith(".html") || l.endsWith(".htm")) fmt_ = 1;
  else if (l.endsWith(".rtf")) fmt_ = 2;
  else fmt_ = 0;  // txt / md / xtc treated as plain
}

ReaderApp::~ReaderApp() { flush(0); }

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
  pageStartMs_ = millis();
}

void ReaderApp::flush(int pagesDelta) {
  uint32_t now = millis();
  uint32_t dwell = now - pageStartMs_;
  if (dwell > READER_DWELL_CAP) dwell = READER_DWELL_CAP;
  Stats::addSession(dwell, pagesDelta);
  pageStartMs_ = now;
}

// Filter one raw byte stream into display text, honoring html/rtf. Returns the
// next character (0 if this position produced none, i still advanced).
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

  auto nextChar = [&](void) -> int {  // returns a display char, ' ', '\n', or -1 (skip)
    char c = buf[i];
    if (fmt_ == 1) {              // HTML: drop tags, keep text
      if (c == '<') {
        // Treat block tags as line breaks.
        size_t tagStart = i + 1;
        while (i < got && buf[i] != '>') ++i;
        if (i < got) ++i;         // consume '>'
        char t = (tagStart < got) ? buf[tagStart] : 0;
        if (t == 'p' || t == 'P' || t == 'b' || t == 'B' || t == 'd' || t == 'D')
          return '\n';
        return ' ';
      }
    } else if (fmt_ == 2) {       // RTF: drop control words and braces
      if (c == '{' || c == '}') { ++i; return -1; }
      if (c == '\\') {
        ++i;
        if (i < got && buf[i] == 'p') return (++i, '\n');  // \par etc.
        while (i < got && isalpha((unsigned char)buf[i])) ++i;
        if (i < got && buf[i] == ' ') ++i;
        return ' ';
      }
    }
    ++i;
    return (unsigned char)c;
  };

  while (lineCount < READER_LINES && i < got) {
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
      // build a word up to a boundary; but nextChar gives one char at a time,
      // so append and wrap on width.
      String cand = line + (char)ch;
      if (duet::textWidth(g, FONT_BODY, cand) <= READER_TEXT_W) {
        line = cand;
      } else {
        // back up i by one char so it starts the next line (only for plain)
        --i;
        break;
      }
    }
    // trim trailing space
    while (line.length() && line[line.length() - 1] == ' ') line.remove(line.length() - 1);
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
    case Btn::Down:
    case Btn::Right:
      if (nextOffset_ < size_) {
        if (idx_ == (int)starts_.size() - 1) starts_.push_back(nextOffset_);
        ++idx_;
        flush(1);
        saveProgress();
        return true;
      }
      return true;
    case Btn::Up:
    case Btn::Left:
      if (idx_ > 0) { --idx_; flush(0); saveProgress(); return true; }
      return true;
    default:
      return false;  // Back -> library
  }
}

void ReaderApp::render(DuetDisplay& d) {
  auto& g = d.gfx();
  int pct = size_ ? (int)((uint64_t)starts_[idx_] * 100 / size_) : 0;
  duet::headerBar(g, duet::fit(g, FONT_LARGE, title_, SCREEN_W - 120),
                  String(pct) + "%");

  if (!ok_) {
    duet::centerText(g, SCREEN_W / 2, SCREEN_H / 2, FONT_MED,
                     "Could not open file.", UI_BLACK);
    duet::buttonBar(g, "Back", "", "", "");
    return;
  }

  std::vector<String> lines;
  layout(d, starts_[idx_], lines);
  int y = READER_TOP;
  for (auto& ln : lines) {
    if (ln.length()) duet::text(g, UI_MARGIN, y, FONT_BODY, ln, UI_BLACK);
    y += READER_LINE_H;
  }
  if (nextOffset_ >= size_ && size_ > 0) Stats::setCompleted(key_, true);

  duet::buttonBar(g, "Back", "", "Prev", "Next");  // side Up/Down also page
}
