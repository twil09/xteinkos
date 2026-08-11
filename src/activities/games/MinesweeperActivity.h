#pragma once
#include <cstdint>

#include "activities/Activity.h"

// Minesweeper: move with the arrows, reveal with Select. Flood-reveals empties;
// hit a mine and it's over. Clear every safe cell to win.
class MinesweeperActivity : public Activity {
 public:
  MinesweeperActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Minesweeper", renderer, mappedInput) {}
  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  static constexpr int COLS = 9, ROWS = 11, MINES = 12;
  bool mine[ROWS][COLS] = {{false}};
  bool shown[ROWS][COLS] = {{false}};
  int curR = 0, curC = 0;
  bool started = false;  // mines placed after the first reveal
  bool over = false, won = false;

  void reset();
  void placeMines(int safeR, int safeC);
  int adjacent(int r, int c) const;
  void reveal(int r, int c);
  bool checkWin() const;
};
