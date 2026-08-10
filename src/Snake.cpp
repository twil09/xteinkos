#include "Snake.h"
#include "theme.h"

void Snake::reset() {
  randomSeed(micros());
  body_.clear();
  int r = ROWS / 2, c = COLS / 2;
  body_.push_back(r * COLS + c);
  body_.push_back(r * COLS + c - 1);
  body_.push_back(r * COLS + c - 2);
  dx_ = pdx_ = 1; dy_ = pdy_ = 0;
  over_ = false;
  score_ = 0;
  placeFood();
}

bool Snake::occupied(int cell) const {
  for (int b : body_) if (b == cell) return true;
  return false;
}

void Snake::placeFood() {
  int tries = 0;
  do {
    food_ = random(ROWS * COLS);
  } while (occupied(food_) && ++tries < 500);
}

bool Snake::onButton(Btn b) {
  switch (b) {
    case Btn::Up:    if (dy_ != 1)  { pdx_ = 0; pdy_ = -1; } return true;
    case Btn::Down:  if (dy_ != -1) { pdx_ = 0; pdy_ = 1; }  return true;
    case Btn::Left:  if (dx_ != 1)  { pdx_ = -1; pdy_ = 0; } return true;
    case Btn::Right: if (dx_ != -1) { pdx_ = 1; pdy_ = 0; }  return true;
    case Btn::Confirm: if (over_) { reset(); return true; } return false;
    default: return false;  // Back -> home
  }
}

bool Snake::onTick() {
  if (over_) return false;
  dx_ = pdx_; dy_ = pdy_;
  int head = body_.front();
  int hr = head / COLS + dy_, hc = head % COLS + dx_;
  if (hr < 0 || hr >= ROWS || hc < 0 || hc >= COLS) { over_ = true; return true; }
  int nh = hr * COLS + hc;
  for (size_t i = 0; i + 1 < body_.size(); ++i)
    if (body_[i] == nh) { over_ = true; return true; }

  body_.insert(body_.begin(), nh);
  if (nh == food_) { ++score_; placeFood(); }  // grow (keep tail)
  else body_.pop_back();
  return true;
}

void Snake::render(DuetDisplay& d) {
  auto& g = d.gfx();
  duet::header(g, over_ ? String("Snake  -  Game over  (") + score_ + ")"
                        : String("Snake    Score: ") + score_);

  const int cell = 24;
  const int x0 = (SCREEN_W - COLS * cell) / 2;
  const int y0 = UI_HEADER_H + 2;
  g.drawRect(x0 - 2, y0 - 2, COLS * cell + 4, ROWS * cell + 4, UI_BLACK);

  if (food_ >= 0) {
    int fr = food_ / COLS, fc = food_ % COLS;
    g.fillCircle(x0 + fc * cell + cell / 2, y0 + fr * cell + cell / 2,
                 cell / 2 - 4, UI_BLACK);
  }
  for (size_t i = 0; i < body_.size(); ++i) {
    int r = body_[i] / COLS, c = body_[i] % COLS;
    int x = x0 + c * cell, y = y0 + r * cell;
    g.fillRect(x + 1, y + 1, cell - 2, cell - 2, UI_BLACK);
    if (i == 0) g.fillRect(x + cell / 2 - 2, y + cell / 2 - 2, 4, 4, UI_WHITE);
  }

  duet::footerHint(g, over_ ? "Confirm: play again    Back: home"
                            : "Arrows: steer    Back: home");
}
