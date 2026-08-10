// FilesApp — splits storage into Books and Images.
#pragma once
#include "App.h"

class FilesApp : public App {
 public:
  bool onButton(Btn b) override;
  void render(DuetDisplay& d) override;
  EInkDisplay::RefreshMode refreshMode() const override {
    return EInkDisplay::HALF_REFRESH;
  }

 private:
  int sel_ = 0;
};
