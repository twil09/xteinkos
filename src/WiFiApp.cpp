#include "WiFiApp.h"

#include <WiFi.h>

#include "Net.h"
#include "QrView.h"
#include "WifiStore.h"
#include "theme.h"

#define AP_NAME "Vix-Setup"
#define MENU_ROWS 3

static const char FORM_HTML[] PROGMEM =
    "<!doctype html><html><head><meta charset=UTF-8>"
    "<meta name=viewport content=\"width=device-width,initial-scale=1\">"
    "<title>Vix OS Wi-Fi</title><style>"
    "body{font-family:'Segoe UI',sans-serif;background:#F5F5F0;color:#2C3E50;padding:16px}"
    ".c{max-width:420px;margin:auto;background:#fff;padding:22px;border-radius:8px;"
    "box-shadow:0 2px 8px rgba(0,0,0,.1)}label{font-weight:600;display:block;margin:12px 0 4px}"
    "input{width:100%;padding:11px;border:1px solid #BDC3C7;border-radius:4px;box-sizing:border-box}"
    "button{width:100%;margin-top:18px;padding:13px;background:#3498DB;color:#fff;border:0;"
    "border-radius:4px;font-weight:700;font-size:1rem}</style></head><body><div class=c>"
    "<h2>&#128246; Vix OS Wi-Fi</h2><p>Enter your home network.</p>"
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

void WiFiApp::beginConnect() {
  Net::connectSaved();
  connectStart_ = millis();
  mode_ = Mode::Connecting;
}

void WiFiApp::enterTransfer() {
  Net::startFileServer();
  mode_ = Mode::Transfer;
}

bool WiFiApp::onTick() {
  if (mode_ == Mode::Portal) {
    dns_.processNextRequest();
    server_.handleClient();
    if (gotConfig_) {
      WifiStore::save(pendingSsid_, pendingPass_);
      stopPortal();
      beginConnect();
      return true;
    }
    return false;
  }
  if (mode_ == Mode::Connecting) {
    if (Net::connected()) { mode_ = Mode::Connected; return true; }
    if (millis() - connectStart_ > 20000) { mode_ = Mode::Failed; return true; }
    return false;
  }
  return false;
}

bool WiFiApp::onButton(Btn b) {
  if (mode_ == Mode::Menu) {
    switch (b) {
      case Btn::Up:   sel_ = (sel_ + MENU_ROWS - 1) % MENU_ROWS; return true;
      case Btn::Down: sel_ = (sel_ + 1) % MENU_ROWS; return true;
      case Btn::Confirm:
        if (sel_ == 0) {                       // Connect / reconnect
          if (Net::connected()) enterTransfer();
          else if (WifiStore::has()) beginConnect();
          else startPortal();
        } else if (sel_ == 1) {                // Set up via phone
          startPortal();
        } else {                               // Transfer files
          if (Net::connected()) enterTransfer();
          else if (WifiStore::has()) beginConnect();
          else startPortal();
        }
        return true;
      default: return false;  // Back -> home
    }
  }
  // Active modes: Back/Confirm/Power returns to the menu (and tidies up).
  if (b == Btn::Back || b == Btn::Confirm || b == Btn::Power) {
    if (mode_ == Mode::Portal) stopPortal();
    if (mode_ == Mode::Transfer) Net::stopFileServer();
    mode_ = Mode::Menu;
    return true;
  }
  return true;
}

void WiFiApp::render(DuetDisplay& d) {
  auto& g = d.gfx();
  duet::headerBar(g, "Wi-Fi", "");

  if (mode_ == Mode::Menu) {
    String ssid, pass;
    bool has = WifiStore::load(ssid, pass);
    String status = Net::connected()
                        ? ("Connected: " + Net::ssid() + "  " + Net::ip())
                        : (has ? ("Saved: " + ssid) : "No network saved");
    duet::text(g, UI_MARGIN, UI_HEADER_H + 34, FONT_MED, status, UI_BLACK);

    const int rowH = 64;
    int y = UI_HEADER_H + 58;
    duet::listRow(g, 0, y, SCREEN_W, rowH,
                  Net::connected() ? "Transfer files (open page)"
                                   : (has ? ("Connect to " + ssid)
                                          : "Connect (none saved)"),
                  sel_ == 0);
    duet::listRow(g, 0, y + rowH, SCREEN_W, rowH, "Set up via phone", sel_ == 1);
    duet::listRow(g, 0, y + 2 * rowH, SCREEN_W, rowH, "Transfer files", sel_ == 2);
    duet::buttonBar(g, "Back", "Select", "Up", "Down");
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
    duet::centerText(g, SCREEN_W / 2, UI_HEADER_H + 60, FONT_LARGE, "Connected!",
                     UI_BLACK);
    duet::centerText(g, SCREEN_W / 2, UI_HEADER_H + 92, FONT_MED,
                     Net::ssid(), UI_BLACK);
    duet::footerHint(g, "Confirm: transfer files   Back: menu");
    return;
  }

  if (mode_ == Mode::Transfer) {
    duet::text(g, UI_MARGIN, UI_HEADER_H + 32, FONT_MED,
               "On your phone (same Wi-Fi), open:", UI_BLACK);
    duet::text(g, UI_MARGIN, UI_HEADER_H + 62, FONT_LARGE, Net::hostUrl(), UI_BLACK);
    duet::text(g, UI_MARGIN, UI_HEADER_H + 88, FONT_BODY,
               "or scan (opens " + Net::url() + ")", UI_BLACK);
    qrview::draw(g, Net::url(), UI_HEADER_H + 106, 260);
    duet::footerHint(g, "Upload books & images from the page   Back: stop");
    return;
  }

  // Failed
  duet::centerText(g, SCREEN_W / 2, SCREEN_H / 2 - 10, FONT_LARGE,
                   "Could not connect", UI_BLACK);
  duet::centerText(g, SCREEN_W / 2, SCREEN_H / 2 + 24, FONT_BODY,
                   "Check the password and try setup again.", UI_BLACK);
  duet::footerHint(g, "Back: menu");
}
