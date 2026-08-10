// Reversi (Othello) — player is black (disc), CPU is white (ring), greedy AI.
#pragma once
#include "App.h"

class Reversi : public App {
 public:
  void onEnter() override { reset(); }
  bool onButton(Btn b) override;
  void render(DuetDisplay& d) override;

 private:
  uint8_t bd_[8][8];  // 0 empty, 1 black (player), 2 white (cpu)
  int cr_ = 2, cc_ = 3;
  int turn_ = 1;      // whose turn
  int result_ = 0;    // 0 playing, else 1/2/3(draw)

  void reset();
  static int other(int p) { return p == 1 ? 2 : 1; }
  int flips(int r, int c, int p, bool apply);
  bool hasMove(int p);
  void cpuTurn();
  void finishIfDone();
  void counts(int& black, int& white) const;
};
