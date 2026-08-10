// Game2048 — the classic 2048 sliding-tile game.
#pragma once
#include "App.h"

class Game2048 : public App {
 public:
  void onEnter() override { reset(); }
  bool onButton(Btn b) override;
  void render(DuetDisplay& d) override;

 private:
  int  board_[4][4] = {{0}};
  long score_ = 0;
  bool over_ = false;
  bool won_ = false;

  void reset();
  void spawn();
  bool slide(Btn dir);       // returns true if the board changed
  bool anyMovesLeft() const;
};
