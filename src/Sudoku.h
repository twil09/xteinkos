// Sudoku — random puzzle (backtracking generator), cursor entry, conflict hints.
#pragma once
#include "App.h"

class Sudoku : public App {
 public:
  void onEnter() override { generate(); }
  bool onButton(Btn b) override;
  void render(DuetDisplay& d) override;

 private:
  uint8_t board_[9][9] = {{0}};
  uint8_t sol_[9][9] = {{0}};
  bool    fixed_[9][9] = {{false}};
  int     cr_ = 0, cc_ = 0;  // cursor row/col

  void generate();
  bool fill(int idx);
  static bool valid(uint8_t b[9][9], int r, int c, int v);
  bool conflict(int r, int c) const;
  bool solved() const;
};
