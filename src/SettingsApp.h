// SettingsApp — device info, clock/time-zone, reading stats, and actions.
#pragma once
#include "App.h"

class SettingsApp : public App {
 public:
  bool onButton(Btn b) override;
  void render(DuetDisplay& d) override;
  EInkDisplay::RefreshMode refreshMode() const override {
    return EInkDisplay::HALF_REFRESH;
  }

 private:
  static const int kRows = 3;
  int sel_ = 0;  // 0 Sleep now, 1 Time zone (Left/Right adjusts), 2 QR codes
};
