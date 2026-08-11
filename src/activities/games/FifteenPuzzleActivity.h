#pragma once
#include "activities/Activity.h"

// 15 Puzzle: slide tiles into the blank with Left/Right/Up/Down (the arrows move
// the blank). Arrange 1..15 in order.
class FifteenPuzzleActivity : public Activity {
 public:
  FifteenPuzzleActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("15 Puzzle", renderer, mappedInput) {}
  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  int tiles[16] = {0};  // value at each position; 0 = blank
  int blank = 15;
  int moves = 0;
  bool solved = false;

  void reset();
  bool moveBlank(int dir);  // 0 left 1 right 2 up 3 down
  bool isSolved() const;
};
