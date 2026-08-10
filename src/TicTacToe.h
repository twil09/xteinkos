// TicTacToe — player is X, CPU is O (perfect minimax play).
#pragma once
#include "App.h"

class TicTacToe : public App {
 public:
  void onEnter() override { reset(); }
  bool onButton(Btn b) override;
  void render(DuetDisplay& d) override;

 private:
  uint8_t board_[9] = {0};  // 0 empty, 1 = X (player), 2 = O (cpu)
  int cursor_ = 4;
  int result_ = 0;  // 0 playing, 1 X wins, 2 O wins, 3 draw

  void reset();
  void cpuMove();
  void evaluate();
  static int winner(const uint8_t b[9]);
  static bool full(const uint8_t b[9]);
  static int minimax(uint8_t b[9], bool aiTurn);
};
