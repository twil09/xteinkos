// SettingsApp — device info, reading stats, and actions (sleep, share*).
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
  int sel_ = 0;  // 0 Sleep now, 1 Share stats, 2 QR codes
};
