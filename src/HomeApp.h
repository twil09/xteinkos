// HomeApp — the Duet-styled app launcher.
#pragma once
#include "App.h"

class HomeApp : public App {
 public:
  bool onButton(Btn b) override;
  void render(DuetDisplay& d) override;
  EInkDisplay::RefreshMode refreshMode() const override {
    return EInkDisplay::HALF_REFRESH;
  }

 private:
  int sel_ = 0;
  int scrollTop_ = 0;
  void launch(int i);
};
