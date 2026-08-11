#pragma once
#include <cstdint>

#include "activities/Activity.h"

// 2048: slide tiles with Left/Right/Up/Down; equal tiles merge.
class Game2048Activity : public Activity {
 public:
  Game2048Activity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("2048", renderer, mappedInput) {}
  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  static constexpr int N = 4;
  int board[N][N] = {{0}};
  long score = 0;
  bool over = false;

  void reset();
  void spawn();
  bool slide(int dir);        // 0 left, 1 right, 2 up, 3 down; returns changed
  bool movesAvailable() const;
};
