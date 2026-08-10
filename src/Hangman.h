// Hangman — guess the word with a letter grid; six misses and you're out.
#pragma once
#include "App.h"

class Hangman : public App {
 public:
  void onEnter() override { reset(); }
  bool onButton(Btn b) override;
  void render(DuetDisplay& d) override;

 private:
  String word_;
  bool guessed_[26];
  int misses_ = 0;
  int cur_ = 0;          // index in the 26-letter grid
  int state_ = 0;        // 0 playing, 1 won, 2 lost
  static const int COLS = 7, ROWS = 4;  // 28 cells (26 used)

  void reset();
  bool revealed(char c) const;
  bool won() const;
};
