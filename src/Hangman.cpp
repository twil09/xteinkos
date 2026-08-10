#include "Hangman.h"
#include "theme.h"

static const char* const WORDS[] = {
    "ARDUINO", "DISPLAY", "BATTERY", "PROCESSOR", "KEYBOARD", "NETWORK",
    "PROGRAM", "VARIABLE", "FUNCTION", "COMPILER", "PIXEL", "ELECTRON",
    "MEMORY", "PROTOCOL", "WIRELESS", "GRAPHITE", "READER", "PUZZLE",
    "MACHINE", "CIRCUIT", "VOLTAGE", "MONITOR", "STORAGE", "SANDWICH",
    "MOUNTAIN"};
static const int WORD_COUNT = sizeof(WORDS) / sizeof(WORDS[0]);

void Hangman::reset() {
  randomSeed(micros());
  word_ = WORDS[random(WORD_COUNT)];
  for (int i = 0; i < 26; ++i) guessed_[i] = false;
  misses_ = 0;
  cur_ = 0;
  state_ = 0;
}

bool Hangman::revealed(char c) const { return guessed_[c - 'A']; }

bool Hangman::won() const {
  for (size_t i = 0; i < word_.length(); ++i)
    if (!revealed(word_[i])) return false;
  return true;
}

bool Hangman::onButton(Btn b) {
  if (state_ != 0) {
    if (b == Btn::Confirm) { reset(); return true; }
    return false;
  }
  int r = cur_ / COLS, c = cur_ % COLS;
  switch (b) {
    case Btn::Up:    if (r > 0) cur_ -= COLS; return true;
    case Btn::Down:  if (r < ROWS - 1 && cur_ + COLS < 26) cur_ += COLS; return true;
    case Btn::Left:  if (cur_ > 0) --cur_; return true;
    case Btn::Right: if (cur_ < 25) ++cur_; return true;
    case Btn::Confirm: {
      if (guessed_[cur_]) return true;
      guessed_[cur_] = true;
      char letter = 'A' + cur_;
      if (word_.indexOf(letter) < 0) {
        if (++misses_ >= 6) state_ = 2;
      } else if (won()) {
        state_ = 1;
      }
      return true;
    }
    default: return false;  // Back -> home
  }
}

void Hangman::render(DuetDisplay& d) {
  auto& g = d.gfx();
  duet::header(g, state_ == 1 ? "Hangman  -  You got it!"
               : state_ == 2 ? "Hangman  -  Out of guesses"
                             : String("Hangman    Misses: ") + misses_ + "/6");

  // Gallows (progressive with misses), top-left.
  int gx = 70, gy = UI_HEADER_H + 30;
  g.drawFastVLine(gx, gy, 150, UI_BLACK);
  g.drawFastHLine(gx, gy, 90, UI_BLACK);
  g.drawFastVLine(gx + 90, gy, 30, UI_BLACK);
  int hx = gx + 90, hy = gy + 30;
  if (misses_ > 0) g.drawCircle(hx, hy + 15, 15, UI_BLACK);       // head
  if (misses_ > 1) g.drawFastVLine(hx, hy + 30, 45, UI_BLACK);    // body
  if (misses_ > 2) g.drawLine(hx, hy + 40, hx - 20, hy + 60, UI_BLACK);  // arm
  if (misses_ > 3) g.drawLine(hx, hy + 40, hx + 20, hy + 60, UI_BLACK);  // arm
  if (misses_ > 4) g.drawLine(hx, hy + 75, hx - 18, hy + 100, UI_BLACK); // leg
  if (misses_ > 5) g.drawLine(hx, hy + 75, hx + 18, hy + 100, UI_BLACK); // leg

  // Word with blanks / revealed letters.
  String shown;
  for (size_t i = 0; i < word_.length(); ++i) {
    char ch = word_[i];
    bool show = revealed(ch) || state_ != 0;
    shown += show ? ch : '_';
    shown += ' ';
  }
  duet::centerText(g, SCREEN_W / 2, UI_HEADER_H + 210, FONT_MED,
                   duet::fit(g, FONT_MED, shown, SCREEN_W - 24), UI_BLACK);

  // Letter grid.
  const int cw = 72, ch = 48, gxr = (SCREEN_W - COLS * cw) / 2, gyr = UI_HEADER_H + 250;
  for (int i = 0; i < 26; ++i) {
    int r = i / COLS, c = i % COLS;
    int x = gxr + c * cw, y = gyr + r * ch;
    bool sel = (i == cur_) && state_ == 0;
    if (sel) g.fillRect(x, y, cw - 4, ch - 4, UI_BLACK);
    else g.drawRect(x, y, cw - 4, ch - 4, UI_BLACK);
    String s = String((char)('A' + i));
    duet::centerText(g, x + (cw - 4) / 2, y + (ch - 4) / 2 + 6, FONT_MED, s,
                     sel ? UI_WHITE : UI_BLACK);
    if (guessed_[i] && !sel)  // strike through used letters
      g.drawFastHLine(x + 6, y + (ch - 4) / 2, cw - 16, UI_BLACK);
  }

  duet::footerHint(g, state_ ? "Confirm: new word    Back: home"
                             : "Arrows: pick   Confirm: guess   Back: home");
}
