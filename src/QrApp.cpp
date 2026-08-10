#include "QrApp.h"

#include <WiFi.h>

#include "QrView.h"
#include "WifiStore.h"
#include "theme.h"

void QrApp::onEnter() {
  labels_.clear();
  payloads_.clear();
  idx_ = 0;

  labels_.push_back("Project: github.com/twil09/xteinkos");
  payloads_.push_back("https://github.com/twil09/xteinkos");

  String ssid, pass;
  if (WifiStore::load(ssid, pass)) {
    labels_.push_back("Join Wi-Fi: " + ssid);
    // Standard WiFi QR (Android/iOS understand this).
    payloads_.push_back("WIFI:T:WPA;S:" + ssid + ";P:" + pass + ";;");
  }

  if (WiFi.status() == WL_CONNECTED) {
    String ip = WiFi.localIP().toString();
    labels_.push_back("This device: http://" + ip);
    payloads_.push_back("http://" + ip);
  }
}

bool QrApp::onButton(Btn b) {
  int n = (int)payloads_.size();
  if (n == 0) return false;
  switch (b) {
    case Btn::Left:  idx_ = (idx_ - 1 + n) % n; return true;
    case Btn::Right: idx_ = (idx_ + 1) % n;     return true;
    default: return false;  // Back -> home
  }
}

void QrApp::render(DuetDisplay& d) {
  auto& g = d.gfx();
  duet::header(g, "QR Codes");

  if (payloads_.empty()) {
    duet::centerText(g, SCREEN_W / 2, SCREEN_H / 2, FONT_MED, "Nothing to show",
                     UI_BLACK);
    duet::footerHint(g, "Back: home");
    return;
  }

  duet::centerText(g, SCREEN_W / 2, UI_HEADER_H + 34, FONT_MED,
                   duet::fit(g, FONT_MED, labels_[idx_], SCREEN_W - 40),
                   UI_BLACK);
  int px = qrview::draw(g, payloads_[idx_], UI_HEADER_H + 56, 320);
  (void)px;
  String pos = String(idx_ + 1) + "/" + String((int)payloads_.size());
  duet::footerHint(g, "Left/Right: switch    " + pos + "    Back: home");
}
