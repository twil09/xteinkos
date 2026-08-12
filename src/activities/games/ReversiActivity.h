#pragma once
#include <cstdint>

#include "activities/Activity.h"

// Reversi / Othello vs a greedy AI. Player = filled disc (1), AI = ring (2).
class ReversiActivity : public Activity {
 public:
  ReversiActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Reversi", renderer, mappedInput) {}
  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  static constexpr int N = 8;
  uint8_t board[N][N] = {{0}};
  int curR = 2, curC = 3;
  bool playerTurn = true;
  bool over = false;

  void reset();
  int  gain(uint8_t who, int r, int c, bool apply);  // flips from placing who at r,c
  bool hasMove(uint8_t who) const;
  void aiTurns();
  int  count(uint8_t who) const;
};
