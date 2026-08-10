// HomeApp — Vix OS launcher: book carousel + a 2x2 section grid.
#pragma once
#include <vector>
#include "App.h"

class HomeApp : public App {
 public:
  void onEnter() override;
  bool onButton(Btn b) override;
  void render(DuetDisplay& d) override;
  EInkDisplay::RefreshMode refreshMode() const override {
    return EInkDisplay::HALF_REFRESH;
  }

 private:
  int sel_ = 0;  // 0=Games 1=Files 2=Wi-Fi 3=Settings (2x2)
  std::vector<String> covers_;    // book display names
  std::vector<uint8_t> done_;     // completed flag
  void scanBooks();
  void launch(int i);
};
