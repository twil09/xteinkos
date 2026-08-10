#include "Sudoku.h"
#include "theme.h"

bool Sudoku::valid(uint8_t b[9][9], int r, int c, int v) {
  for (int i = 0; i < 9; ++i)
    if (b[r][i] == v || b[i][c] == v) return false;
  int br = (r / 3) * 3, bc = (c / 3) * 3;
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      if (b[br + i][bc + j] == v) return false;
  return true;
}

bool Sudoku::fill(int idx) {
  if (idx == 81) return true;
  int r = idx / 9, c = idx % 9;
  if (sol_[r][c] != 0) return fill(idx + 1);
  uint8_t nums[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  for (int i = 8; i > 0; --i) {  // Fisher-Yates shuffle
    int j = random(i + 1);
    uint8_t t = nums[i]; nums[i] = nums[j]; nums[j] = t;
  }
  for (int i = 0; i < 9; ++i) {
    if (valid(sol_, r, c, nums[i])) {
      sol_[r][c] = nums[i];
      if (fill(idx + 1)) return true;
      sol_[r][c] = 0;
    }
  }
  return false;
}

void Sudoku::generate() {
  randomSeed(micros());
  for (int r = 0; r < 9; ++r)
    for (int c = 0; c < 9; ++c) sol_[r][c] = 0;
  fill(0);
  for (int r = 0; r < 9; ++r)
    for (int c = 0; c < 9; ++c) { board_[r][c] = sol_[r][c]; fixed_[r][c] = true; }
  int toRemove = 48;
  while (toRemove > 0) {
    int r = random(9), c = random(9);
    if (board_[r][c] != 0) { board_[r][c] = 0; fixed_[r][c] = false; --toRemove; }
  }
  cr_ = cc_ = 0;
}

bool Sudoku::conflict(int r, int c) const {
  int v = board_[r][c];
  if (v == 0) return false;
  for (int i = 0; i < 9; ++i) {
    if (i != c && board_[r][i] == v) return true;
    if (i != r && board_[i][c] == v) return true;
  }
  int br = (r / 3) * 3, bc = (c / 3) * 3;
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      if ((br + i != r || bc + j != c) && board_[br + i][bc + j] == v) return true;
  return false;
}

bool Sudoku::solved() const {
  for (int r = 0; r < 9; ++r)
    for (int c = 0; c < 9; ++c)
      if (board_[r][c] != sol_[r][c]) return false;
  return true;
}

bool Sudoku::onButton(Btn b) {
  switch (b) {
    case Btn::Up:    cr_ = (cr_ + 8) % 9; return true;
    case Btn::Down:  cr_ = (cr_ + 1) % 9; return true;
    case Btn::Left:  cc_ = (cc_ + 8) % 9; return true;
    case Btn::Right: cc_ = (cc_ + 1) % 9; return true;
    case Btn::Confirm:
      if (!fixed_[cr_][cc_]) board_[cr_][cc_] = (board_[cr_][cc_] + 1) % 10;
      return true;
    default: return false;  // Back -> home
  }
}

void Sudoku::render(DuetDisplay& d) {
  auto& g = d.gfx();
  duet::header(g, solved() ? "Sudoku  -  Solved!" : "Sudoku");

  const int cell = 50, n = 9;
  const int boardSz = cell * n;
  const int x0 = (SCREEN_W - boardSz) / 2;
  const int y0 = UI_HEADER_H + 8;

  for (int r = 0; r < n; ++r)
    for (int c = 0; c < n; ++c) {
      int x = x0 + c * cell, y = y0 + r * cell;
      g.drawRect(x, y, cell, cell, UI_BLACK);
      if (r == cr_ && c == cc_) {
        g.drawRect(x + 1, y + 1, cell - 2, cell - 2, UI_BLACK);
        g.drawRect(x + 2, y + 2, cell - 4, cell - 4, UI_BLACK);
      }
      int v = board_[r][c];
      if (v) {
        const GFXfont* f = fixed_[r][c] ? FONT_MED : FONT_BODY;
        duet::centerText(g, x + cell / 2, y + cell / 2 + 7, f, String(v), UI_BLACK);
        if (conflict(r, c))
          g.drawFastHLine(x + 10, y + cell - 8, cell - 20, UI_BLACK);
      }
    }
  for (int i = 0; i <= 9; i += 3) {
    g.drawFastHLine(x0, y0 + i * cell, boardSz, UI_BLACK);
    g.drawFastHLine(x0, y0 + i * cell - 1, boardSz, UI_BLACK);
    g.drawFastVLine(x0 + i * cell, y0, boardSz, UI_BLACK);
    g.drawFastVLine(x0 + i * cell - 1, y0, boardSz, UI_BLACK);
  }
  duet::footerHint(g, "Arrows: move   Confirm: cycle 1-9   Back: home");
}
