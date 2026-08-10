// QrApp — cycle scannable QR codes (project link, Wi-Fi share, device IP).
#pragma once
#include <vector>
#include "App.h"

class QrApp : public App {
 public:
  void onEnter() override;
  bool onButton(Btn b) override;
  void render(DuetDisplay& d) override;
  EInkDisplay::RefreshMode refreshMode() const override {
    return EInkDisplay::HALF_REFRESH;
  }

 private:
  std::vector<String> labels_;
  std::vector<String> payloads_;
  int idx_ = 0;
};
