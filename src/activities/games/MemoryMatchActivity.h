#pragma once
#include "activities/Activity.h"

// Memory Match: flip cards to find all 8 pairs on a 4x4 board.
class MemoryMatchActivity : public Activity {
 public:
  MemoryMatchActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Memory", renderer, mappedInput) {}
  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  static constexpr int COLS = 4, ROWS = 4, COUNT = COLS * ROWS;
  int value[COUNT] = {0};
  bool matched[COUNT] = {false};
  int cursor = 0;
  int firstPick = -1;
  int pendingA = -1, pendingB = -1;  // a shown mismatch, dismissed on next input
  int pairsFound = 0;
  bool won = false;

  void reset();
  bool shown(int i) const;
};
