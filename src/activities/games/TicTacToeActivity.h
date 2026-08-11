#pragma once
#include <cstdint>

#include "activities/Activity.h"

// Tic-Tac-Toe vs a simple AI. Player is X, AI is O.
class TicTacToeActivity : public Activity {
 public:
  TicTacToeActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("TicTacToe", renderer, mappedInput) {}
  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  uint8_t board[9] = {0};  // 0 empty, 1 X (player), 2 O (AI)
  int cursor = 4;
  uint8_t winner = 0;   // 0 none/ongoing, 1 X, 2 O
  bool over = false;    // true on win or draw
  bool draw = false;

  void reset();
  void placeAndRespond(int cell);
  void aiMove();
  uint8_t lineWinner() const;         // 1/2 if a line is complete, else 0
  bool boardFull() const;
  int findWinningMove(uint8_t who) const;  // cell that completes a line for `who`, or -1
};
