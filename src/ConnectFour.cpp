#include "ConnectFour.h"
#include "theme.h"

void ConnectFour::reset() {
  for (int r = 0; r < ROWS; ++r)
    for (int c = 0; c < COLS; ++c) grid_[r][c] = 0;
  player_ = 1;
  col_ = COLS / 2;
  winner_ = 0;
}

bool ConnectFour::drop(int c) {
  for (int r = ROWS - 1; r >= 0; --r)
    if (grid_[r][c] == 0) { grid_[r][c] = player_; return true; }
  return false;
}

bool ConnectFour::full() const {
  for (int c = 0; c < COLS; ++c)
    if (grid_[0][c] == 0) return false;
  return true;
}

int ConnectFour::checkWin() const {
  const int dirs[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
  for (int r = 0; r < ROWS; ++r)
    for (int c = 0; c < COLS; ++c) {
      int p = grid_[r][c];
      if (!p) continue;
      for (auto& dr : dirs) {
        int k = 1;
        while (k < 4) {
          int nr = r + dr[0] * k, nc = c + dr[1] * k;
          if (nr < 0 || nr >= ROWS || nc < 0 || nc >= COLS || grid_[nr][nc] != p) break;
          ++k;
        }
        if (k == 4) return p;
      }
    }
  return 0;
}

bool ConnectFour::onButton(Btn b) {
  if (winner_ != 0) {
    if (b == Btn::Confirm) { reset(); return true; }
    return false;
  }
  switch (b) {
    case Btn::Left:  col_ = (col_ + COLS - 1) % COLS; return true;
    case Btn::Right: col_ = (col_ + 1) % COLS;        return true;
    case Btn::Confirm:
      if (drop(col_)) {
        int w = checkWin();
        if (w) winner_ = w;
        else if (full()) winner_ = 3;
        else player_ = (player_ == 1) ? 2 : 1;
      }
      return true;
    default: return false;  // Back -> home
  }
}

void ConnectFour::render(DuetDisplay& d) {
  auto& g = d.gfx();
  String h;
  if (winner_ == 3) h = "Connect Four  -  Draw";
  else if (winner_) h = String("Connect Four  -  P") + winner_ + " wins!";
  else h = String("Connect Four  -  P") + player_ + " (" +
           (player_ == 1 ? "disc" : "ring") + ")";
  duet::header(g, h);

  const int cell = 66;
  const int boardW = COLS * cell, boardH = ROWS * cell;
  const int x0 = (SCREEN_W - boardW) / 2;
  const int y0 = UI_HEADER_H + 40;

  if (winner_ == 0) {
    int ax = x0 + col_ * cell + cell / 2;
    g.fillTriangle(ax - 10, y0 - 30, ax + 10, y0 - 30, ax, y0 - 12, UI_BLACK);
  }

  g.drawRect(x0 - 2, y0 - 2, boardW + 4, boardH + 4, UI_BLACK);
  for (int r = 0; r < ROWS; ++r)
    for (int c = 0; c < COLS; ++c) {
      int x = x0 + c * cell, y = y0 + r * cell;
      g.drawRect(x, y, cell, cell, UI_BLACK);
      int p = grid_[r][c];
      int cx = x + cell / 2, cy = y + cell / 2, rad = cell / 2 - 8;
      if (p == 1) {
        g.fillCircle(cx, cy, rad, UI_BLACK);
      } else if (p == 2) {
        g.drawCircle(cx, cy, rad, UI_BLACK);
        g.drawCircle(cx, cy, rad - 1, UI_BLACK);
      }
    }

  duet::footerHint(g, winner_ ? "Confirm: new game    Back: home"
                              : "Left/Right: column   Confirm: drop   Back: home");
}
