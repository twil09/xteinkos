// PlaceholderApp — a simple "coming soon" screen for features not built yet.
#pragma once
#include "App.h"
#include "theme.h"

class PlaceholderApp : public App {
 public:
  explicit PlaceholderApp(const String& name, const String& note = "")
      : name_(name), note_(note) {}

  void render(DuetDisplay& d) override {
    auto& g = d.gfx();
    duet::header(g, name_);
    duet::centerText(g, SCREEN_W / 2, SCREEN_H / 2 - 10, FONT_LARGE,
                     "Coming soon", UI_BLACK);
    if (note_.length())
      duet::centerText(g, SCREEN_W / 2, SCREEN_H / 2 + 24, FONT_BODY, note_,
                       UI_BLACK);
    duet::footerHint(g, "Back: return home");
  }
  EInkDisplay::RefreshMode refreshMode() const override {
    return EInkDisplay::HALF_REFRESH;
  }

 private:
  String name_, note_;
};
