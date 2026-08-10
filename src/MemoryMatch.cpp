#include "MemoryMatch.h"
#include "theme.h"

void MemoryMatch::reset() {
  randomSeed(micros());
  for (int i = 0; i < N; ++i) { val_[i] = i / 2; up_[i] = false; done_[i] = false; }
  for (int i = N - 1; i > 0; --i) {  // shuffle
    int j = random(i + 1);
    int t = val_[i]; val_[i] = val_[j]; val_[j] = t;
  }
  cur_ = 0; first_ = -1; pendingHide_ = false; hideA_ = hideB_ = -1; moves_ = 0;
}

bool MemoryMatch::won() const {
  for (int i = 0; i < N; ++i) if (!done_[i]) return false;
  return true;
}

void MemoryMatch::clearMismatch() {
  if (pendingHide_) {
    if (hideA_ >= 0) up_[hideA_] = false;
    if (hideB_ >= 0) up_[hideB_] = false;
    pendingHide_ = false; hideA_ = hideB_ = -1;
  }
}

bool MemoryMatch::onButton(Btn b) {
  if (pendingHide_) { clearMismatch(); if (b == Btn::Confirm) return true; }

  int r = cur_ / COLS, c = cur_ % COLS;
  switch (b) {
    case Btn::Up:    if (r > 0) cur_ -= COLS; return true;
    case Btn::Down:  if (r < ROWS - 1) cur_ += COLS; return true;
    case Btn::Left:  if (c > 0) cur_ -= 1; return true;
    case Btn::Right: if (c < COLS - 1) cur_ += 1; return true;
    case Btn::Confirm: {
      if (won()) { reset(); return true; }
      if (done_[cur_] || up_[cur_]) return true;
      up_[cur_] = true;
      if (first_ == -1) {
        first_ = cur_;
      } else {
        ++moves_;
        if (val_[first_] == val_[cur_]) {
          done_[first_] = true; done_[cur_] = true;
          first_ = -1;
        } else {
          pendingHide_ = true; hideA_ = first_; hideB_ = cur_;
          first_ = -1;
        }
      }
      return true;
    }
    default: return false;  // Back -> home
  }
}

void MemoryMatch::render(DuetDisplay& d) {
  auto& g = d.gfx();
  duet::header(g, won() ? String("Memory  -  Done in ") + moves_ + " tries"
                        : String("Memory    Tries: ") + moves_);

  const int cw = 120, ch = 100, gx = 4, gy = 4;
  const int boardW = COLS * cw + (COLS - 1) * gx;
  const int x0 = (SCREEN_W - boardW) / 2;
  const int y0 = UI_HEADER_H + 8;

  for (int i = 0; i < N; ++i) {
    int r = i / COLS, c = i % COLS;
    int x = x0 + c * (cw + gx), y = y0 + r * (ch + gy);
    g.drawRect(x, y, cw, ch, UI_BLACK);
    if (done_[i] || up_[i]) {
      String s = String((char)('A' + val_[i]));
      duet::centerText(g, x + cw / 2, y + ch / 2 + 10, FONT_LARGE, s, UI_BLACK);
      if (done_[i]) g.drawRect(x + 3, y + 3, cw - 6, ch - 6, UI_BLACK);
    } else {
      duet::centerText(g, x + cw / 2, y + ch / 2 + 10, FONT_LARGE, "?", UI_BLACK);
    }
    if (i == cur_) {
      g.drawRect(x + 1, y + 1, cw - 2, ch - 2, UI_BLACK);
      g.drawRect(x + 2, y + 2, cw - 4, ch - 4, UI_BLACK);
    }
  }
  duet::footerHint(g, "Arrows: move   Confirm: flip   Back: home");
}
