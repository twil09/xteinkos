// LibraryApp — list books or images (SD + LittleFS) and open the right viewer.
#pragma once
#include <vector>
#include "App.h"

class LibraryApp : public App {
 public:
  enum class Cat { Books, Images };
  explicit LibraryApp(Cat cat = Cat::Books) : cat_(cat) {}
  void onEnter() override { scan(); }
  bool onButton(Btn b) override;
  void render(DuetDisplay& d) override;
  EInkDisplay::RefreshMode refreshMode() const override {
    return EInkDisplay::HALF_REFRESH;
  }

 private:
  Cat cat_;
  std::vector<String> paths_;
  std::vector<String> names_;
  std::vector<uint8_t> fromSd_;
  int sel_ = 0, scrollTop_ = 0;
  void scan();
  void addDir(bool sd, const char* dir);
  void openSelected();
};
