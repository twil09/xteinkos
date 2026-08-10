// ConnectFour — 2-player local. Left/Right pick a column, Confirm drops.
#pragma once
#include "App.h"

class ConnectFour : public App {
 public:
  static const int COLS = 7, ROWS = 6;
  void onEnter() override { reset(); }
  bool onButton(Btn b) override;
  void render(DuetDisplay& d) override;

 private:
  uint8_t grid_[ROWS][COLS];  // 0 empty, 1 = P1, 2 = P2
  int player_ = 1;
  int col_ = COLS / 2;
  int winner_ = 0;  // 0 none, 1/2 winner, 3 draw

  void reset();
  bool drop(int c);
  int  checkWin() const;
  bool full() const;
};
