#include "Reversi.h"
#include "theme.h"

static const int DIRS[8][2] = {{-1,-1},{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1}};

void Reversi::reset() {
  for (int r = 0; r < 8; ++r)
    for (int c = 0; c < 8; ++c) bd_[r][c] = 0;
  bd_[3][3] = bd_[4][4] = 2;
  bd_[3][4] = bd_[4][3] = 1;
  cr_ = 2; cc_ = 3;
  turn_ = 1;
  result_ = 0;
}

// Count (and optionally perform) flips for placing p at (r,c). 0 = illegal.
int Reversi::flips(int r, int c, int p, bool apply) {
  if (bd_[r][c] != 0) return 0;
  int total = 0;
  int opp = other(p);
  for (auto& d : DIRS) {
    int rr = r + d[0], cc = c + d[1], cnt = 0;
    while (rr >= 0 && rr < 8 && cc >= 0 && cc < 8 && bd_[rr][cc] == opp) {
      rr += d[0]; cc += d[1]; ++cnt;
    }
    if (cnt > 0 && rr >= 0 && rr < 8 && cc >= 0 && cc < 8 && bd_[rr][cc] == p) {
      total += cnt;
      if (apply) {
        int fr = r + d[0], fc = c + d[1];
        for (int i = 0; i < cnt; ++i) { bd_[fr][fc] = p; fr += d[0]; fc += d[1]; }
      }
    }
  }
  if (apply && total > 0) bd_[r][c] = p;
  return total;
}

bool Reversi::hasMove(int p) {
  for (int r = 0; r < 8; ++r)
    for (int c = 0; c < 8; ++c)
      if (bd_[r][c] == 0 && flips(r, c, p, false) > 0) return true;
  return false;
}

void Reversi::counts(int& black, int& white) const {
  black = white = 0;
  for (int r = 0; r < 8; ++r)
    for (int c = 0; c < 8; ++c) {
      if (bd_[r][c] == 1) ++black;
      else if (bd_[r][c] == 2) ++white;
    }
}

void Reversi::finishIfDone() {
  if (!hasMove(1) && !hasMove(2)) {
    int b, w; counts(b, w);
    result_ = (b > w) ? 1 : (w > b) ? 2 : 3;
  }
}

void Reversi::cpuTurn() {
  // Play greedy moves for white until it's the player's turn (or game ends).
  while (turn_ == 2 && result_ == 0) {
    int bestR = -1, bestC = -1, best = 0;
    for (int r = 0; r < 8; ++r)
      for (int c = 0; c < 8; ++c) {
        int f = (bd_[r][c] == 0) ? flips(r, c, 2, false) : 0;
        if (f > best) { best = f; bestR = r; bestC = c; }
      }
    if (bestR >= 0) flips(bestR, bestC, 2, true);
    turn_ = 1;
    if (!hasMove(1)) {          // player must pass
      if (hasMove(2)) turn_ = 2;  // white plays again
      else { finishIfDone(); return; }
    }
  }
}

bool Reversi::onButton(Btn b) {
  if (result_ != 0) {
    if (b == Btn::Confirm) { reset(); return true; }
    return false;
  }
  switch (b) {
    case Btn::Up:    cr_ = (cr_ + 7) % 8; return true;
    case Btn::Down:  cr_ = (cr_ + 1) % 8; return true;
    case Btn::Left:  cc_ = (cc_ + 7) % 8; return true;
    case Btn::Right: cc_ = (cc_ + 1) % 8; return true;
    case Btn::Confirm:
      if (turn_ == 1 && flips(cr_, cc_, 1, false) > 0) {
        flips(cr_, cc_, 1, true);
        turn_ = 2;
        cpuTurn();
        finishIfDone();
      }
      return true;
    default: return false;  // Back -> home
  }
}

void Reversi::render(DuetDisplay& d) {
  auto& g = d.gfx();
  int black, white; counts(black, white);
  String h;
  if (result_ == 1) h = "Reversi  -  You win!";
  else if (result_ == 2) h = "Reversi  -  CPU wins";
  else if (result_ == 3) h = "Reversi  -  Draw";
  else h = String("Reversi   You ") + black + " : " + white + " CPU";
  duet::header(g, h);

  const int cell = 50, n = 8;
  const int boardSz = cell * n;
  const int x0 = (SCREEN_W - boardSz) / 2;
  const int y0 = UI_HEADER_H + 16;

  for (int r = 0; r < n; ++r)
    for (int c = 0; c < n; ++c) {
      int x = x0 + c * cell, y = y0 + r * cell;
      g.drawRect(x, y, cell, cell, UI_BLACK);
      int cx = x + cell / 2, cy = y + cell / 2, rad = cell / 2 - 8;
      if (bd_[r][c] == 1) g.fillCircle(cx, cy, rad, UI_BLACK);
      else if (bd_[r][c] == 2) { g.drawCircle(cx, cy, rad, UI_BLACK); g.drawCircle(cx, cy, rad - 1, UI_BLACK); }
      if (r == cr_ && c == cc_ && result_ == 0) {
        bool legal = bd_[r][c] == 0 && flips(r, c, 1, false) > 0;
        g.drawRect(x + 2, y + 2, cell - 4, cell - 4, UI_BLACK);
        if (legal) g.drawRect(x + 4, y + 4, cell - 8, cell - 8, UI_BLACK);
      }
    }
  duet::footerHint(g, result_ ? "Confirm: new game    Back: home"
                              : "Arrows: move   Confirm: place   Back: home");
}
