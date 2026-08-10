// Snake — grid-based, auto-advancing via timed ticks. Arrows steer.
#pragma once
#include <vector>
#include "App.h"

class Snake : public App {
 public:
  static const int COLS = 21, ROWS = 28;
  void onEnter() override { reset(); }
  bool onButton(Btn b) override;
  bool onTick() override;
  uint32_t tickIntervalMs() const override { return over_ ? 0 : 170; }
  void render(DuetDisplay& d) override;

 private:
  std::vector<int> body_;  // cell indices, front = head
  int dx_ = 1, dy_ = 0;
  int pdx_ = 1, pdy_ = 0;  // pending direction
  int food_ = -1;
  bool over_ = false;
  int score_ = 0;

  void reset();
  void placeFood();
  bool occupied(int cell) const;
};
