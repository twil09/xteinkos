#pragma once
#include <cstdint>

#include "activities/Activity.h"

// Sudoku with a randomly generated puzzle. Left/Right/Up/Down move the cursor;
// Confirm cycles the value in a non-clue cell (blank -> 1..9 -> blank).
class SudokuActivity : public Activity {
 public:
  SudokuActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Sudoku", renderer, mappedInput) {}
  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  static constexpr int HOLES = 45;  // cells cleared for the player to fill

  uint8_t grid[81] = {0};      // current board, 0 = empty
  uint8_t solution[81] = {0};  // the generated full solution
  bool fixed[81] = {false};    // true for the given clues (immutable)
  int curR = 0, curC = 0;
  bool won = false;

  void generate();
  bool fillGrid(int pos);                  // backtracking full-solution filler
  bool canPlace(int pos, uint8_t v) const;  // no row/col/box conflict
  bool solved() const;
};
