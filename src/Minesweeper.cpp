#include "Minesweeper.h"
#include "theme.h"

void Minesweeper::reset() {
  randomSeed(micros());
  for (int r = 0; r < ROWS; ++r)
    for (int c = 0; c < COLS; ++c) {
      mine_[r][c] = open_[r][c] = flag_[r][c] = false;
      adj_[r][c] = 0;
    }
  cr_ = ROWS / 2; cc_ = COLS / 2;
  state_ = 0;
  placed_ = false;
}

void Minesweeper::placeMines(int safeR, int safeC) {
  int placed = 0;
  while (placed < MINES) {
    int r = random(ROWS), c = random(COLS);
    if (mine_[r][c]) continue;
    if (r == safeR && c == safeC) continue;  // first click is safe
    mine_[r][c] = true;
    ++placed;
  }
  for (int r = 0; r < ROWS; ++r)
    for (int c = 0; c < COLS; ++c) {
      int n = 0;
      for (int dr = -1; dr <= 1; ++dr)
        for (int dc = -1; dc <= 1; ++dc) {
          int nr = r + dr, nc = c + dc;
          if (nr >= 0 && nr < ROWS && nc >= 0 && nc < COLS && mine_[nr][nc]) ++n;
        }
      adj_[r][c] = n;
    }
  placed_ = true;
}

void Minesweeper::reveal(int r, int c) {
  if (r < 0 || r >= ROWS || c < 0 || c >= COLS) return;
  if (open_[r][c] || flag_[r][c]) return;
  open_[r][c] = true;
  if (adj_[r][c] == 0 && !mine_[r][c]) {
    for (int dr = -1; dr <= 1; ++dr)
      for (int dc = -1; dc <= 1; ++dc)
        if (dr || dc) reveal(r + dr, c + dc);
  }
}

bool Minesweeper::won() const {
  for (int r = 0; r < ROWS; ++r)
    for (int c = 0; c < COLS; ++c)
      if (!mine_[r][c] && !open_[r][c]) return false;
  return true;
}

bool Minesweeper::onButton(Btn b) {
  if (state_ != 0) {
    if (b == Btn::Confirm || b == Btn::Back) { reset(); return true; }
    return false;  // Power -> home
  }
  switch (b) {
    case Btn::Up:    cr_ = (cr_ + ROWS - 1) % ROWS; return true;
    case Btn::Down:  cr_ = (cr_ + 1) % ROWS;        return true;
    case Btn::Left:  cc_ = (cc_ + COLS - 1) % COLS; return true;
    case Btn::Right: cc_ = (cc_ + 1) % COLS;        return true;
    case Btn::Back:  // toggle flag
      if (!open_[cr_][cc_]) flag_[cr_][cc_] = !flag_[cr_][cc_];
      return true;
    case Btn::Confirm:
      if (flag_[cr_][cc_]) return true;
      if (!placed_) placeMines(cr_, cc_);
      if (mine_[cr_][cc_]) { open_[cr_][cc_] = true; state_ = 1; }
      else { reveal(cr_, cc_); if (won()) state_ = 2; }
      return true;
    default: return false;  // Power -> home
  }
}

void Minesweeper::render(DuetDisplay& d) {
  auto& g = d.gfx();
  const char* h = state_ == 1 ? "Minesweeper  -  Boom!"
                : state_ == 2 ? "Minesweeper  -  Cleared!"
                              : "Minesweeper";
  duet::header(g, h);

  const int cell = 50;
  const int x0 = (SCREEN_W - COLS * cell) / 2;
  const int y0 = UI_HEADER_H + 6;

  for (int r = 0; r < ROWS; ++r)
    for (int c = 0; c < COLS; ++c) {
      int x = x0 + c * cell, y = y0 + r * cell;
      bool showMine = mine_[r][c] && (state_ == 1);
      if (open_[r][c] || showMine) {
        g.drawRect(x, y, cell, cell, UI_BLACK);
        if (mine_[r][c]) {
          g.fillCircle(x + cell / 2, y + cell / 2, 9, UI_BLACK);
        } else if (adj_[r][c] > 0) {
          duet::centerText(g, x + cell / 2, y + cell / 2 + 7, FONT_MED,
                           String(adj_[r][c]), UI_BLACK);
        }
      } else {
        g.fillRect(x, y, cell, cell, UI_WHITE);
        g.drawRect(x, y, cell, cell, UI_BLACK);
        g.drawRect(x + 1, y + 1, cell - 2, cell - 2, UI_BLACK);
        if (flag_[r][c])
          duet::centerText(g, x + cell / 2, y + cell / 2 + 7, FONT_MED, "F", UI_BLACK);
      }
      if (r == cr_ && c == cc_ && state_ == 0) {
        g.drawRect(x + 3, y + 3, cell - 6, cell - 6, UI_BLACK);
        g.drawRect(x + 4, y + 4, cell - 8, cell - 8, UI_BLACK);
      }
    }

  duet::footerHint(g, state_ == 0
                          ? "Arrows move  Confirm reveal  Back flag  Power exit"
                          : "Confirm: new game    Power: exit");
}
