// FifteenPuzzle — classic 4x4 sliding puzzle. Arrows move the blank.
#pragma once
#include "App.h"

class FifteenPuzzle : public App {
 public:
  void onEnter() override { reset(); }
  bool onButton(Btn b) override;
  void render(DuetDisplay& d) override;

 private:
  int tile_[16];  // 0 = blank
  int blank_ = 15;
  int moves_ = 0;

  void reset();
  bool moveBlank(int dr, int dc);
  bool solved() const;
};
