// OpdsApp — browse saved OPDS catalogs and download EPUBs over Wi-Fi.
#pragma once
#include <vector>
#include "App.h"
#include "Opds.h"
#include "OpdsStore.h"

class OpdsApp : public App {
 public:
  void onEnter() override;
  bool onButton(Btn b) override;
  bool onTick() override;
  uint32_t tickIntervalMs() const override { return pending_ ? 10 : 0; }
  void render(DuetDisplay& d) override;
  bool keepAwake() const override { return true; }  // network activity
  EInkDisplay::RefreshMode refreshMode() const override {
    return EInkDisplay::HALF_REFRESH;
  }

 private:
  enum class Mode { Servers, Loading, Feed, Working, Message };
  Mode mode_ = Mode::Servers;

  std::vector<OpdsServer> servers_;
  Opds::Feed feed_;
  std::vector<String> stack_;   // back-navigation URLs
  String curUrl_, pendingUrl_, msg_;
  Opds::Entry pendingBook_;
  bool pending_ = false;        // a fetch/download is queued for onTick
  bool pendingIsDownload_ = false;
  int sel_ = 0, scrollTop_ = 0;

  void loadFeed(const String& url, bool push);
  void renderList(DuetDisplay& d, const char* title);
};
