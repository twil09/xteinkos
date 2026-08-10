// GamesApp — the Games "folder": a scrolling list of all games.
#pragma once
#include "App.h"

class GamesApp : public App {
 public:
  bool onButton(Btn b) override;
  void render(DuetDisplay& d) override;
  EInkDisplay::RefreshMode refreshMode() const override {
    return EInkDisplay::HALF_REFRESH;
  }

 private:
  int sel_ = 0, scrollTop_ = 0;
  void launch(int i);
};
