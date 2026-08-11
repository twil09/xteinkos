// WiFiApp — connect to a saved network, set one up from a phone (captive AP),
// or transfer files to/from a phone over the current Wi-Fi.
#pragma once
#include <DNSServer.h>
#include <WebServer.h>
#include "App.h"

class WiFiApp : public App {
 public:
  enum class Mode { Menu, Portal, Connecting, Connected, Failed, Transfer };

  void onEnter() override;
  bool onButton(Btn b) override;
  bool onTick() override;
  uint32_t tickIntervalMs() const override {
    return (mode_ == Mode::Portal || mode_ == Mode::Connecting) ? 40 : 0;
  }
  void render(DuetDisplay& d) override;
  EInkDisplay::RefreshMode refreshMode() const override {
    return EInkDisplay::HALF_REFRESH;
  }
  bool keepAwake() const override { return mode_ != Mode::Menu; }

 private:
  Mode      mode_ = Mode::Menu;
  int       sel_ = 0;
  uint32_t  connectStart_ = 0;
  bool      gotConfig_ = false;
  String    pendingSsid_, pendingPass_;
  DNSServer dns_;
  WebServer server_{80};
  IPAddress apIP_{192, 168, 4, 1};

  void startPortal();
  void stopPortal();
  void beginConnect();
  void enterTransfer();
  void routes_();
};
