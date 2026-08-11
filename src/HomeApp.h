// HomeApp — Vix OS launcher: a scrolling book carousel + 4 sections.
//  Left/Right step through the sections (Games, Books, Wi-Fi, Settings);
//  the side Up/Down buttons scroll the "Continue reading" covers.
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
  static const int kSections = 4;
  int sel_ = 0;          // 0=Games 1=Books 2=Wi-Fi 3=Settings (linear)
  int carousel_ = 0;     // first visible cover
  std::vector<String> covers_;    // book display names
  std::vector<uint8_t> done_;     // completed flag
  void scanBooks();
  void launch(int i);
};
