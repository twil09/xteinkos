#include "Game2048.h"
#include "theme.h"

void Game2048::reset() {
  randomSeed(micros());
  for (int r = 0; r < 4; ++r)
    for (int c = 0; c < 4; ++c) board_[r][c] = 0;
  score_ = 0;
  over_ = false;
  won_ = false;
  spawn();
  spawn();
}

void Game2048::spawn() {
  int empties[16][2];
  int n = 0;
  for (int r = 0; r < 4; ++r)
    for (int c = 0; c < 4; ++c)
      if (board_[r][c] == 0) { empties[n][0] = r; empties[n][1] = c; ++n; }
  if (n == 0) return;
  int k = random(n);
  board_[empties[k][0]][empties[k][1]] = (random(10) == 0) ? 4 : 2;
}

bool Game2048::slide(Btn dir) {
  bool changed = false;
  auto slideToFront = [&](int line[4]) -> bool {
    int tmp[4], n = 0;
    for (int i = 0; i < 4; ++i) if (line[i] != 0) tmp[n++] = line[i];
    for (int i = n; i < 4; ++i) tmp[i] = 0;
    for (int i = 0; i < 3; ++i) {
      if (tmp[i] != 0 && tmp[i] == tmp[i + 1]) {
        tmp[i] *= 2;
        score_ += tmp[i];
        if (tmp[i] >= 2048) won_ = true;
        tmp[i + 1] = 0;
      }
    }
    int out[4], m = 0;
    for (int i = 0; i < 4; ++i) if (tmp[i] != 0) out[m++] = tmp[i];
    for (int i = m; i < 4; ++i) out[i] = 0;
    bool ch = false;
    for (int i = 0; i < 4; ++i) { if (line[i] != out[i]) ch = true; line[i] = out[i]; }
    return ch;
  };

  for (int k = 0; k < 4; ++k) {
    int line[4];
    for (int i = 0; i < 4; ++i) {
      switch (dir) {
        case Btn::Left:  line[i] = board_[k][i];     break;
        case Btn::Right: line[i] = board_[k][3 - i]; break;
        case Btn::Up:    line[i] = board_[i][k];     break;
        default:         line[i] = board_[3 - i][k]; break;  // Down
      }
    }
    if (slideToFront(line)) changed = true;
    for (int i = 0; i < 4; ++i) {
      switch (dir) {
        case Btn::Left:  board_[k][i] = line[i];     break;
        case Btn::Right: board_[k][3 - i] = line[i]; break;
        case Btn::Up:    board_[i][k] = line[i];     break;
        default:         board_[3 - i][k] = line[i]; break;  // Down
      }
    }
  }
  return changed;
}

bool Game2048::anyMovesLeft() const {
  for (int r = 0; r < 4; ++r)
    for (int c = 0; c < 4; ++c) {
      if (board_[r][c] == 0) return true;
      if (c < 3 && board_[r][c] == board_[r][c + 1]) return true;
      if (r < 3 && board_[r][c] == board_[r + 1][c]) return true;
    }
  return false;
}

bool Game2048::onButton(Btn b) {
  switch (b) {
    case Btn::Up:
    case Btn::Down:
    case Btn::Left:
    case Btn::Right: {
      if (over_) return false;
      bool moved = slide(b);
      if (moved) {
        spawn();
        if (!anyMovesLeft()) over_ = true;
      }
      return moved;
    }
    case Btn::Confirm:
      reset();
      return true;
    default:
      return false;  // Back/Power -> manager pops to home
  }
}

void Game2048::render(DuetDisplay& d) {
  auto& g = d.gfx();
  duet::header(g, String("2048    Score: ") + score_);

  const int cell = 88, gap = 8, n = 4;
  const int boardW = n * cell + (n - 1) * gap;
  const int x0 = (SCREEN_W - boardW) / 2;
  const int y0 = UI_HEADER_H + 26;

  for (int r = 0; r < n; ++r) {
    for (int c = 0; c < n; ++c) {
      int x = x0 + c * (cell + gap);
      int y = y0 + r * (cell + gap);
      g.drawRect(x, y, cell, cell, UI_BLACK);
      int v = board_[r][c];
      if (v) {
        if (v >= 128) {
          g.fillRect(x + 2, y + 2, cell - 4, cell - 4, UI_BLACK);
          duet::centerText(g, x + cell / 2, y + cell / 2 + 7, FONT_MED,
                           String(v), UI_WHITE);
        } else {
          duet::centerText(g, x + cell / 2, y + cell / 2 + 7, FONT_MED,
                           String(v), UI_BLACK);
        }
      }
    }
  }

  if (over_) {
    duet::centerText(g, SCREEN_W / 2, y0 + boardW + 22, FONT_MED, "Game over!",
                     UI_BLACK);
  } else if (won_) {
    duet::centerText(g, SCREEN_W / 2, y0 + boardW + 22, FONT_MED,
                     "2048! keep going", UI_BLACK);
  }
  duet::footerHint(g, "Arrows: move   Confirm: new game   Back: home");
}
