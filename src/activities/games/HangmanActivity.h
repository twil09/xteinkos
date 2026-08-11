#pragma once
#include "activities/Activity.h"

// Hangman: guess the hidden word letter by letter before running out of misses.
class HangmanActivity : public Activity {
 public:
  HangmanActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Hangman", renderer, mappedInput) {}
  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  static constexpr int MAX_WRONG = 6;
  static constexpr int COLS = 7;  // alphabet keyboard columns
  char word[20] = {0};
  bool guessed[26] = {false};
  int wrong = 0;
  int cursor = 0;  // 0..25
  int state = 0;   // 0 playing, 1 won, 2 lost

  void reset();
  void guessLetter(int l);
  bool isWin() const;
};
