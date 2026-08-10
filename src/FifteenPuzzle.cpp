#include "FifteenPuzzle.h"
#include "theme.h"

void FifteenPuzzle::reset() {
  randomSeed(micros());
  for (int i = 0; i < 16; ++i) tile_[i] = (i + 1) % 16;  // 1..15,0
  blank_ = 15;
  moves_ = 0;
  const int dirs[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
  for (int i = 0; i < 400; ++i) {  // shuffle by valid blank moves (solvable)
    int k = random(4);
    moveBlank(dirs[k][0], dirs[k][1]);
  }
  moves_ = 0;
}

bool FifteenPuzzle::moveBlank(int dr, int dc) {
  int br = blank_ / 4, bc = blank_ % 4;
  int nr = br + dr, nc = bc + dc;
  if (nr < 0 || nr >= 4 || nc < 0 || nc >= 4) return false;
  int ni = nr * 4 + nc;
  tile_[blank_] = tile_[ni];
  tile_[ni] = 0;
  blank_ = ni;
  ++moves_;
  return true;
}

bool FifteenPuzzle::solved() const {
  for (int i = 0; i < 15; ++i)
    if (tile_[i] != i + 1) return false;
  return tile_[15] == 0;
}

bool FifteenPuzzle::onButton(Btn b) {
  switch (b) {
    case Btn::Up:    return moveBlank(-1, 0);
    case Btn::Down:  return moveBlank(1, 0);
    case Btn::Left:  return moveBlank(0, -1);
    case Btn::Right: return moveBlank(0, 1);
    case Btn::Confirm: reset(); return true;
    default: return false;
  }
}

void FifteenPuzzle::render(DuetDisplay& d) {
  auto& g = d.gfx();
  duet::header(g, solved() ? "15 Puzzle  -  Solved!"
                           : String("15 Puzzle    Moves: ") + moves_);

  const int cell = 96, n = 4, gap = 6;
  const int boardW = n * cell + (n - 1) * gap;
  const int x0 = (SCREEN_W - boardW) / 2;
  const int y0 = UI_HEADER_H + 26;

  for (int i = 0; i < 16; ++i) {
    int r = i / 4, c = i % 4;
    int x = x0 + c * (cell + gap), y = y0 + r * (cell + gap);
    if (tile_[i] == 0) continue;
    g.drawRect(x, y, cell, cell, UI_BLACK);
    g.drawRect(x + 1, y + 1, cell - 2, cell - 2, UI_BLACK);
    duet::centerText(g, x + cell / 2, y + cell / 2 + 8, FONT_LARGE,
                     String(tile_[i]), UI_BLACK);
  }
  duet::footerHint(g, "Arrows: slide   Confirm: shuffle   Back: home");
}
