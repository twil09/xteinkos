// ReaderApp — paginated .txt/.md reader (SD or LittleFS), with saved progress.
#pragma once
#include <vector>
#include "App.h"
#include "BookSource.h"

class ReaderApp : public App {
 public:
  ReaderApp(const String& path, bool fromSd = false);
  void onEnter() override;
  bool onButton(Btn b) override;
  void render(DuetDisplay& d) override;
  EInkDisplay::RefreshMode refreshMode() const override {
    return EInkDisplay::HALF_REFRESH;
  }

 private:
  String   path_, title_, key_;
  bool     fromSd_ = false;
  BookSource src_;
  uint32_t size_ = 0;
  bool     ok_ = false;

  std::vector<uint32_t> starts_;
  int      idx_ = 0;
  uint32_t nextOffset_ = 0;

  void layout(DuetDisplay& d, uint32_t start, std::vector<String>& lines);
  void saveProgress();
};
