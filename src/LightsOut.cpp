#include "LightsOut.h"
#include "theme.h"

void LightsOut::press(int r, int c) {
  const int d[5][2] = {{0, 0}, {-1, 0}, {1, 0}, {0, -1}, {0, 1}};
  for (auto& o : d) {
    int nr = r + o[0], nc = c + o[1];
    if (nr >= 0 && nr < N && nc >= 0 && nc < N) on_[nr][nc] = !on_[nr][nc];
  }
}

void LightsOut::reset() {
  randomSeed(micros());
  for (int r = 0; r < N; ++r)
    for (int c = 0; c < N; ++c) on_[r][c] = false;
  for (int i = 0; i < 12; ++i) press(random(N), random(N));  // solvable scramble
  cr_ = cc_ = 0;
  presses_ = 0;
}

bool LightsOut::solved() const {
  for (int r = 0; r < N; ++r)
    for (int c = 0; c < N; ++c)
      if (on_[r][c]) return false;
  return true;
}

bool LightsOut::onButton(Btn b) {
  switch (b) {
    case Btn::Up:    cr_ = (cr_ + N - 1) % N; return true;
    case Btn::Down:  cr_ = (cr_ + 1) % N;     return true;
    case Btn::Left:  cc_ = (cc_ + N - 1) % N; return true;
    case Btn::Right: cc_ = (cc_ + 1) % N;     return true;
    case Btn::Confirm:
      if (solved()) { reset(); return true; }
      press(cr_, cc_);
      ++presses_;
      return true;
    default: return false;
  }
}

void LightsOut::render(DuetDisplay& d) {
  auto& g = d.gfx();
  duet::header(g, solved() ? "Lights Out  -  Solved!"
                           : String("Lights Out    Moves: ") + presses_);

  const int cell = 84, gap = 8;
  const int boardW = N * cell + (N - 1) * gap;
  const int x0 = (SCREEN_W - boardW) / 2;
  const int y0 = UI_HEADER_H + 30;

  for (int r = 0; r < N; ++r)
    for (int c = 0; c < N; ++c) {
      int x = x0 + c * (cell + gap), y = y0 + r * (cell + gap);
      if (on_[r][c]) g.fillRect(x, y, cell, cell, UI_BLACK);
      else g.drawRect(x, y, cell, cell, UI_BLACK);
      if (r == cr_ && c == cc_) {
        uint16_t col = on_[r][c] ? UI_WHITE : UI_BLACK;
        g.drawRect(x + 6, y + 6, cell - 12, cell - 12, col);
        g.drawRect(x + 7, y + 7, cell - 14, cell - 14, col);
      }
    }
  duet::footerHint(g, "Arrows: move   Confirm: toggle   Back: home");
}
