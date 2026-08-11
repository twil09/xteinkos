#pragma once
#include "activities/Activity.h"

// Snake: steer with the arrows, eat food to grow, don't hit the walls or
// yourself. Auto-advances on a timer.
class SnakeActivity : public Activity {
 public:
  SnakeActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Snake", renderer, mappedInput) {}
  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;
  bool skipLoopDelay() override { return true; }      // run the loop fast enough to tick
  bool preventAutoSleep() override { return true; }   // don't sleep mid-game

 private:
  static constexpr int COLS = 15, ROWS = 19, MAX = COLS * ROWS;
  int bx[MAX], by[MAX];   // body cells; index 0 = head
  int len = 3;
  int dir = 3, nextDir = 3;  // 0 up, 1 down, 2 left, 3 right
  int foodX = 0, foodY = 0;
  int score = 0;
  bool over = false;
  unsigned long lastStep = 0;

  void reset();
  void spawnFood();
  void step();
  bool onSnake(int x, int y, int upto) const;
};
