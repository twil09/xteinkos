#pragma once
#include <cstdint>

#include "activities/Activity.h"

// Connect Four vs a simple AI. Player = filled disc, AI = ring.
class ConnectFourActivity : public Activity {
 public:
  ConnectFourActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("ConnectFour", renderer, mappedInput) {}
  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  static constexpr int COLS = 7, ROWS = 6;
  uint8_t board[ROWS][COLS] = {{0}};  // 0 empty, 1 player, 2 AI
  int cursorCol = 3;
  bool over = false;
  uint8_t winner = 0;  // 0 none/draw, 1 player, 2 AI
  bool draw = false;

  void reset();
  int dropRow(int c) const;   // lowest empty row in column, or -1 if full
  bool place(int c, uint8_t who);
  bool wins(uint8_t who) const;
  bool boardFull() const;
  void aiMove();
};
