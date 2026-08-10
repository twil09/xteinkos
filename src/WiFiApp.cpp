#include "WiFiApp.h"

#include <WiFi.h>

#include "QrView.h"
#include "WifiStore.h"
#include "theme.h"

#define AP_NAME "xteinkOS-Setup"

static const char FORM_HTML[] PROGMEM =
    "<!doctype html><html><head><meta charset=UTF-8>"
    "<meta name=viewport content=\"width=device-width,initial-scale=1\">"
    "<title>xteinkOS Wi-Fi</title><style>"
    "body{font-family:'Segoe UI',sans-serif;background:#F5F5F0;color:#2C3E50;padding:16px}"
    ".c{max-width:420px;margin:auto;background:#fff;padding:22px;border-radius:8px;"
    "box-shadow:0 2px 8px rgba(0,0,0,.1)}label{font-weight:600;display:block;margin:12px 0 4px}"
    "input{width:100%;padding:11px;border:1px solid #BDC3C7;border-radius:4px;box-sizing:border-box}"
    "button{width:100%;margin-top:18px;padding:13px;background:#3498DB;color:#fff;border:0;"
    "border-radius:4px;font-weight:700;font-size:1rem}</style></head><body><div class=c>"
    "<h2>&#128246; xteinkOS Wi-Fi</h2><p>Enter your home network.</p>"
    "<form action=\"/save\" method=\"get\">"
    "<label>Network name (SSID)</label><input name=\"ssid\" autocapitalize=off>"
    "<label>Password</label><input name=\"pwd\" type=\"password\">"
    "<button type=\"submit\">Save &amp; connect</button></form></div></body></html>";

void WiFiApp::onEnter() {
  mode_ = Mode::Menu;
  sel_ = 0;
}

void WiFiApp::routes_() {
  server_.on("/", HTTP_GET, [this]() { server_.send_P(200, "text/html", FORM_HTML); });
  server_.on("/save", HTTP_GET, [this]() {
    pendingSsid_ = server_.arg("ssid");
    pendingPass_ = server_.arg("pwd");
    gotConfig_ = true;
    server_.send(200, "text/html",
                 "<html><body style='font-family:sans-serif'>"
                 "<h2>Saved. You can return to your device.</h2></body></html>");
  });
  server_.onNotFound([this]() {  // captive redirect
    server_.sendHeader("Location", String("http://") + apIP_.toString() + "/", true);
    server_.send(302, "text/plain", "");
  });
}

void WiFiApp::startPortal() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP_, apIP_, IPAddress(255, 255, 255, 0));
  WiFi.softAP(AP_NAME);  // open network for easy joining
  dns_.start(53, "*", apIP_);
  routes_();
  server_.begin();
  gotConfig_ = false;
  mode_ = Mode::Portal;
}

void WiFiApp::stopPortal() {
  server_.stop();
  dns_.stop();
  WiFi.softAPdisconnect(true);
}

void WiFiApp::startConnect() {
  String ssid, pass;
  if (!WifiStore::load(ssid, pass)) { mode_ = Mode::Failed; return; }
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  connectStart_ = millis();
  mode_ = Mode::Connecting;
}

bool WiFiApp::onTick() {
  if (mode_ == Mode::Portal) {
    dns_.processNextRequest();
    server_.handleClient();
    if (gotConfig_) {
      WifiStore::save(pendingSsid_, pendingPass_);
      stopPortal();
      startConnect();
      return true;
    }
    return false;
  }
  if (mode_ == Mode::Connecting) {
    if (WiFi.status() == WL_CONNECTED) { mode_ = Mode::Connected; return true; }
    if (millis() - connectStart_ > 20000) { mode_ = Mode::Failed; return true; }
    return false;
  }
  return false;
}

bool WiFiApp::onButton(Btn b) {
  if (mode_ == Mode::Menu) {
    switch (b) {
      case Btn::Up:   sel_ = (sel_ + 1) % 2; return true;
      case Btn::Down: sel_ = (sel_ + 1) % 2; return true;
      case Btn::Confirm:
        if (sel_ == 0) { if (WifiStore::has()) startConnect(); }
        else startPortal();
        return true;
      default: return false;  // Back -> home
    }
  }
  // Any of the active modes: Back/Confirm returns to the menu.
  if (b == Btn::Back || b == Btn::Confirm || b == Btn::Power) {
    if (mode_ == Mode::Portal) stopPortal();
    mode_ = Mode::Menu;
    return true;
  }
  return true;
}

void WiFiApp::render(DuetDisplay& d) {
  auto& g = d.gfx();
  duet::header(g, "Wi-Fi Setup");

  if (mode_ == Mode::Menu) {
    String ssid, pass;
    bool has = WifiStore::load(ssid, pass);
    String status = (WiFi.status() == WL_CONNECTED)
                        ? ("Connected: " + WiFi.localIP().toString())
                        : (has ? ("Saved: " + ssid) : "No network saved");
    duet::text(g, UI_MARGIN, UI_HEADER_H + 34, FONT_MED, status, UI_BLACK);

    const int rowH = 66;
    int y = UI_HEADER_H + 60;
    duet::listRow(g, 0, y, SCREEN_W, rowH,
                  has ? ("Connect to " + ssid) : "Connect (none saved)", sel_ == 0);
    duet::listRow(g, 0, y + rowH, SCREEN_W, rowH, "Set up via phone", sel_ == 1);
    duet::footerHint(g, "Up/Down: select   Confirm: go   Back: home");
    return;
  }

  if (mode_ == Mode::Portal) {
    duet::text(g, UI_MARGIN, UI_HEADER_H + 34, FONT_MED,
               "1. Join Wi-Fi:  " AP_NAME, UI_BLACK);
    duet::text(g, UI_MARGIN, UI_HEADER_H + 62, FONT_BODY,
               "2. A page opens (or visit 192.168.4.1)", UI_BLACK);
    duet::text(g, UI_MARGIN, UI_HEADER_H + 86, FONT_BODY,
               "3. Enter your home Wi-Fi and Save", UI_BLACK);
    qrview::draw(g, "WIFI:S:" AP_NAME ";T:nopass;;", UI_HEADER_H + 104, 240);
    duet::footerHint(g, "Scan to join the setup network   Back: cancel");
    return;
  }

  if (mode_ == Mode::Connecting) {
    duet::centerText(g, SCREEN_W / 2, SCREEN_H / 2, FONT_LARGE, "Connecting...",
                     UI_BLACK);
    duet::footerHint(g, "Back: cancel");
    return;
  }

  if (mode_ == Mode::Connected) {
    duet::centerText(g, SCREEN_W / 2, SCREEN_H / 2 - 10, FONT_LARGE, "Connected!",
                     UI_BLACK);
    duet::centerText(g, SCREEN_W / 2, SCREEN_H / 2 + 24, FONT_MED,
                     "IP: " + WiFi.localIP().toString(), UI_BLACK);
    duet::footerHint(g, "Back: menu");
    return;
  }

  // Failed
  duet::centerText(g, SCREEN_W / 2, SCREEN_H / 2 - 10, FONT_LARGE,
                   "Could not connect", UI_BLACK);
  duet::centerText(g, SCREEN_W / 2, SCREEN_H / 2 + 24, FONT_BODY,
                   "Check the password and try setup again.", UI_BLACK);
  duet::footerHint(g, "Back: menu");
}
