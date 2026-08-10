// MemoryMatch — flip two cards to find matching pairs.
#pragma once
#include "App.h"

class MemoryMatch : public App {
 public:
  static const int COLS = 4, ROWS = 6, N = COLS * ROWS;  // 12 pairs
  void onEnter() override { reset(); }
  bool onButton(Btn b) override;
  void render(DuetDisplay& d) override;

 private:
  int  val_[N];
  bool up_[N];       // temporarily face-up
  bool done_[N];     // matched
  int  cur_ = 0;
  int  first_ = -1;  // first pick this turn
  bool pendingHide_ = false;
  int  hideA_ = -1, hideB_ = -1;
  int  moves_ = 0;

  void reset();
  bool won() const;
  void clearMismatch();
};
