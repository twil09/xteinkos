// Minesweeper — Confirm reveals, Back flags, Power exits.
#pragma once
#include "App.h"

class Minesweeper : public App {
 public:
  static const int COLS = 12, ROWS = 8, MINES = 12;
  void onEnter() override { reset(); }
  bool onButton(Btn b) override;
  void render(DuetDisplay& d) override;

 private:
  bool mine_[ROWS][COLS];
  bool open_[ROWS][COLS];
  bool flag_[ROWS][COLS];
  int  adj_[ROWS][COLS];
  int  cr_ = ROWS / 2, cc_ = COLS / 2;
  int  state_ = 0;  // 0 playing, 1 lost, 2 won
  bool placed_ = false;

  void reset();
  void placeMines(int safeR, int safeC);
  void reveal(int r, int c);
  bool won() const;
};
