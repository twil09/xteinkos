// LightsOut — toggle a cell and its neighbors; turn them all off.
#pragma once
#include "App.h"

class LightsOut : public App {
 public:
  static const int N = 5;
  void onEnter() override { reset(); }
  bool onButton(Btn b) override;
  void render(DuetDisplay& d) override;

 private:
  bool on_[N][N];
  int cr_ = 0, cc_ = 0;
  int presses_ = 0;

  void reset();
  void press(int r, int c);
  bool solved() const;
};
