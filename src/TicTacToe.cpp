#include "TicTacToe.h"
#include "theme.h"

void TicTacToe::reset() {
  for (int i = 0; i < 9; ++i) board_[i] = 0;
  cursor_ = 4;
  result_ = 0;
}

int TicTacToe::winner(const uint8_t b[9]) {
  static const int L[8][3] = {{0,1,2},{3,4,5},{6,7,8},{0,3,6},
                              {1,4,7},{2,5,8},{0,4,8},{2,4,6}};
  for (auto& l : L)
    if (b[l[0]] && b[l[0]] == b[l[1]] && b[l[1]] == b[l[2]]) return b[l[0]];
  return 0;
}

bool TicTacToe::full(const uint8_t b[9]) {
  for (int i = 0; i < 9; ++i) if (!b[i]) return false;
  return true;
}

int TicTacToe::minimax(uint8_t b[9], bool aiTurn) {
  int w = winner(b);
  if (w == 2) return 10;
  if (w == 1) return -10;
  if (full(b)) return 0;

  if (aiTurn) {
    int best = -1000;
    for (int i = 0; i < 9; ++i)
      if (!b[i]) { b[i] = 2; int v = minimax(b, false); b[i] = 0; if (v > best) best = v; }
    return best;
  } else {
    int best = 1000;
    for (int i = 0; i < 9; ++i)
      if (!b[i]) { b[i] = 1; int v = minimax(b, true); b[i] = 0; if (v < best) best = v; }
    return best;
  }
}

void TicTacToe::cpuMove() {
  int bestScore = -1000, bestI = -1;
  for (int i = 0; i < 9; ++i) {
    if (!board_[i]) {
      board_[i] = 2;
      int s = minimax(board_, false);
      board_[i] = 0;
      if (s > bestScore) { bestScore = s; bestI = i; }
    }
  }
  if (bestI >= 0) board_[bestI] = 2;
}

void TicTacToe::evaluate() {
  int w = winner(board_);
  if (w == 1) result_ = 1;
  else if (w == 2) result_ = 2;
  else if (full(board_)) result_ = 3;
  else result_ = 0;
}

bool TicTacToe::onButton(Btn b) {
  int row = cursor_ / 3, col = cursor_ % 3;
  switch (b) {
    case Btn::Up:    if (row > 0) cursor_ -= 3; return true;
    case Btn::Down:  if (row < 2) cursor_ += 3; return true;
    case Btn::Left:  if (col > 0) cursor_ -= 1; return true;
    case Btn::Right: if (col < 2) cursor_ += 1; return true;
    case Btn::Confirm:
      if (result_ != 0) { reset(); return true; }
      if (board_[cursor_] == 0) {
        board_[cursor_] = 1;  // player X
        evaluate();
        if (result_ == 0) { cpuMove(); evaluate(); }
        return true;
      }
      return true;
    default:
      return false;  // Back/Power -> pop
  }
}

void TicTacToe::render(DuetDisplay& d) {
  auto& g = d.gfx();
  duet::header(g, "Tic-Tac-Toe");

  const int cell = 118, n = 3;
  const int boardW = cell * n;
  const int x0 = (SCREEN_W - boardW) / 2;
  const int y0 = UI_HEADER_H + 40;

  for (int i = 0; i < 9; ++i) {
    int r = i / 3, c = i % 3;
    int x = x0 + c * cell, y = y0 + r * cell;
    g.drawRect(x, y, cell, cell, UI_BLACK);
    if (i == cursor_ && result_ == 0) {
      g.drawRect(x + 3, y + 3, cell - 6, cell - 6, UI_BLACK);
      g.drawRect(x + 4, y + 4, cell - 8, cell - 8, UI_BLACK);
    }
    int pad = 26;
    if (board_[i] == 1) {  // X
      g.drawLine(x + pad, y + pad, x + cell - pad, y + cell - pad, UI_BLACK);
      g.drawLine(x + cell - pad, y + pad, x + pad, y + cell - pad, UI_BLACK);
      g.drawLine(x + pad + 1, y + pad, x + cell - pad + 1, y + cell - pad, UI_BLACK);
      g.drawLine(x + cell - pad - 1, y + pad, x + pad - 1, y + cell - pad, UI_BLACK);
    } else if (board_[i] == 2) {  // O
      g.drawCircle(x + cell / 2, y + cell / 2, cell / 2 - pad, UI_BLACK);
      g.drawCircle(x + cell / 2, y + cell / 2, cell / 2 - pad - 1, UI_BLACK);
    }
  }

  String status;
  switch (result_) {
    case 1: status = "You win!  Confirm: play again"; break;
    case 2: status = "CPU wins.  Confirm: play again"; break;
    case 3: status = "Draw.  Confirm: play again"; break;
    default: status = "Your move (X)"; break;
  }
  duet::centerText(g, SCREEN_W / 2, y0 + boardW + 30, FONT_MED, status, UI_BLACK);
  duet::footerHint(g, "Arrows: move   Confirm: place   Back: home");
}
