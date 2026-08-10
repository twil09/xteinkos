// Blackjack — hit/stand vs the dealer (dealer hits below 17).
#pragma once
#include <vector>
#include "App.h"

class Blackjack : public App {
 public:
  void onEnter() override { deal(); }
  bool onButton(Btn b) override;
  void render(DuetDisplay& d) override;

 private:
  std::vector<int> player_;  // ranks 1..13 (1=Ace)
  std::vector<int> dealer_;
  int state_ = 0;  // 0 player turn, 1 dealer/settled
  int result_ = 0; // 1 win, 2 lose, 3 push, 4 blackjack (set when state_==1)

  void deal();
  int draw();
  static int handValue(const std::vector<int>& h);
  void stand();
};
