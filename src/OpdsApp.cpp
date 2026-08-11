#include "OpdsApp.h"

#include <WiFi.h>

#include "AppManager.h"
#include "theme.h"

void OpdsApp::onEnter() {
  servers_ = OpdsStore::list();
  mode_ = Mode::Servers;
  sel_ = 0; scrollTop_ = 0;
}

void OpdsApp::loadFeed(const String& url, bool push) {
  if (push && curUrl_.length()) stack_.push_back(curUrl_);
  pendingUrl_ = url;
  pending_ = true;
  pendingIsDownload_ = false;
  mode_ = Mode::Loading;
}

bool OpdsApp::onTick() {
  if (!pending_) return false;
  pending_ = false;
  if (pendingIsDownload_) {
    String saved, err;
    bool ok = Opds::download(pendingBook_.href, pendingBook_.title, saved, err);
    msg_ = ok ? ("Saved to Library:\n" + saved) : ("Download failed:\n" + err);
    mode_ = Mode::Message;
    return true;
  }
  feed_ = Opds::fetch(pendingUrl_);
  curUrl_ = pendingUrl_;
  sel_ = 0; scrollTop_ = 0;
  if (!feed_.ok) { msg_ = "Could not load catalog:\n" + feed_.error; mode_ = Mode::Message; }
  else mode_ = Mode::Feed;
  return true;
}

bool OpdsApp::onButton(Btn b) {
  if (mode_ == Mode::Loading || mode_ == Mode::Working) return true;  // busy

  if (mode_ == Mode::Message) {  // any key returns to the feed/servers
    mode_ = feed_.ok ? Mode::Feed : Mode::Servers;
    return true;
  }

  if (mode_ == Mode::Servers) {
    int n = (int)servers_.size();
    switch (b) {
      case Btn::Up:   if (n) sel_ = (sel_ + n - 1) % n; return true;
      case Btn::Down: if (n) sel_ = (sel_ + 1) % n; return true;
      case Btn::Confirm:
        if (n) { stack_.clear(); curUrl_ = ""; loadFeed(servers_[sel_].url, false); }
        return true;
      default: return false;  // Back -> exit app
    }
  }

  // Feed mode
  int total = (int)feed_.entries.size() + (feed_.nextUrl.length() ? 1 : 0);
  switch (b) {
    case Btn::Up:   if (total) sel_ = (sel_ + total - 1) % total; return true;
    case Btn::Down: if (total) sel_ = (sel_ + 1) % total; return true;
    case Btn::Confirm: {
      if (sel_ == (int)feed_.entries.size() && feed_.nextUrl.length()) {
        loadFeed(feed_.nextUrl, true);  // Next page
        return true;
      }
      if (sel_ < (int)feed_.entries.size()) {
        Opds::Entry& e = feed_.entries[sel_];
        if (e.type == Opds::EntryType::Nav) loadFeed(e.href, true);
        else {
          pendingBook_ = e; pendingIsDownload_ = true; pending_ = true;
          mode_ = Mode::Working;
        }
      }
      return true;
    }
    case Btn::Back:
      if (!stack_.empty()) { String u = stack_.back(); stack_.pop_back(); loadFeed(u, false); return true; }
      mode_ = Mode::Servers; sel_ = 0; scrollTop_ = 0; return true;
    default: return true;
  }
}

void OpdsApp::renderList(DuetDisplay& d, const char* title) {
  auto& g = d.gfx();
  duet::headerBar(g, title, "");
  const int rowH = 58;
  const int rows = (SCREEN_H - UI_HEADER_H - UI_FOOTER_H) / rowH;

  int total;
  auto label = [&](int i) -> String {
    if (mode_ == Mode::Servers) return servers_[i].name;
    if (i < (int)feed_.entries.size()) {
      const Opds::Entry& e = feed_.entries[i];
      return (e.type == Opds::EntryType::Nav ? String("> ") : String("+ ")) + e.title;
    }
    return String(">> Next page");
  };
  total = (mode_ == Mode::Servers)
              ? (int)servers_.size()
              : (int)feed_.entries.size() + (feed_.nextUrl.length() ? 1 : 0);

  if (total == 0) {
    duet::centerText(g, SCREEN_W / 2, SCREEN_H / 2, FONT_MED, "Nothing here", UI_BLACK);
    duet::buttonBar(g, "Back", "", "", "");
    return;
  }
  if (sel_ < scrollTop_) scrollTop_ = sel_;
  if (sel_ >= scrollTop_ + rows) scrollTop_ = sel_ - rows + 1;
  if (scrollTop_ < 0) scrollTop_ = 0;

  int y = UI_HEADER_H;
  for (int i = 0; i < rows; ++i) {
    int idx = scrollTop_ + i;
    if (idx >= total) break;
    duet::listRow(g, 0, y, SCREEN_W, rowH, label(idx), idx == sel_);
    y += rowH;
  }
  duet::buttonBar(g, "Back", "Open", "Up", "Down");
}

void OpdsApp::render(DuetDisplay& d) {
  auto& g = d.gfx();
  if (mode_ == Mode::Servers) { renderList(d, "Download books"); return; }
  if (mode_ == Mode::Feed) {
    String t = feed_.title.length() ? feed_.title : String("Catalog");
    static char buf[64];
    t.toCharArray(buf, sizeof(buf));
    renderList(d, buf);
    return;
  }
  if (mode_ == Mode::Loading) {
    duet::headerBar(g, "Download books", "");
    duet::centerText(g, SCREEN_W / 2, SCREEN_H / 2, FONT_LARGE, "Loading...", UI_BLACK);
    duet::footerHint(g, "Fetching catalog over Wi-Fi");
    return;
  }
  if (mode_ == Mode::Working) {
    duet::headerBar(g, "Downloading", "");
    duet::centerText(g, SCREEN_W / 2, SCREEN_H / 2 - 10, FONT_LARGE, "Downloading...", UI_BLACK);
    duet::centerText(g, SCREEN_W / 2, SCREEN_H / 2 + 22, FONT_BODY,
                     duet::fit(g, FONT_BODY, pendingBook_.title, SCREEN_W - 40), UI_BLACK);
    return;
  }
  // Message
  duet::headerBar(g, "Download books", "");
  int y = UI_HEADER_H + 60;
  int nl = msg_.indexOf('\n');
  duet::centerText(g, SCREEN_W / 2, y, FONT_MED,
                   nl < 0 ? msg_ : msg_.substring(0, nl), UI_BLACK);
  if (nl >= 0)
    duet::centerText(g, SCREEN_W / 2, y + 30, FONT_BODY,
                     duet::fit(g, FONT_BODY, msg_.substring(nl + 1), SCREEN_W - 40), UI_BLACK);
  duet::footerHint(g, "Any button: back");
}
