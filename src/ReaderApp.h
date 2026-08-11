// ReaderApp — paginated reader (SD or LittleFS) with saved progress, adjustable
// fonts/layout, and per-book bookmarks. Confirm opens the in-book menu.
#pragma once
#include <vector>
#include "App.h"
#include "BookSource.h"
#include "BookmarkStore.h"

class ReaderApp : public App {
 public:
  ReaderApp(const String& path, bool fromSd = false);
  ~ReaderApp();
  void onEnter() override;
  bool onButton(Btn b) override;
  void render(DuetDisplay& d) override;
  EInkDisplay::RefreshMode refreshMode() const override {
    return EInkDisplay::HALF_REFRESH;
  }

 private:
  enum class RMode { Reading, Menu, Fonts, Bookmarks };

  String   path_, title_, key_, err_;
  bool     fromSd_ = false;
  bool     epub_ = false;
  int      fmt_ = 0;  // 0 plain, 1 html, 2 rtf
  BookSource src_;
  uint32_t size_ = 0;
  bool     ok_ = false;

  std::vector<uint32_t> starts_;
  int      idx_ = 0;
  uint32_t nextOffset_ = 0;
  String   firstLine_;  // for bookmark summaries

  uint32_t pageStartMs_ = 0;

  RMode    rmode_ = RMode::Reading;
  int      menuSel_ = 0, fontSel_ = 0, bmSel_ = 0;
  std::vector<Bookmark> bms_;

  void layout(DuetDisplay& d, uint32_t start, std::vector<String>& lines);
  void saveProgress();
  void flush(int pagesDelta);
  void relayoutHere();     // keep current offset, drop stale page history
  void jumpTo(uint32_t offset);
  bool pageForward();
  bool pageBack();

  bool onButtonReading(Btn b);
  bool onButtonMenu(Btn b);
  bool onButtonFonts(Btn b);
  bool onButtonBookmarks(Btn b);
  void renderReading(DuetDisplay& d);
  void renderMenu(DuetDisplay& d);
  void renderFonts(DuetDisplay& d);
  void renderBookmarks(DuetDisplay& d);
};
