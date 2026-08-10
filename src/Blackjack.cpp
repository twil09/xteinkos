#include "Blackjack.h"
#include "theme.h"

int Blackjack::draw() { return 1 + random(13); }  // 1..13

int Blackjack::handValue(const std::vector<int>& h) {
  int sum = 0, aces = 0;
  for (int r : h) {
    int v = (r >= 10) ? 10 : r;
    if (r == 1) { v = 11; ++aces; }
    sum += v;
  }
  while (sum > 21 && aces > 0) { sum -= 10; --aces; }
  return sum;
}

void Blackjack::deal() {
  randomSeed(micros());
  player_.clear();
  dealer_.clear();
  player_.push_back(draw());
  player_.push_back(draw());
  dealer_.push_back(draw());
  dealer_.push_back(draw());
  state_ = 0;
  result_ = 0;
  if (handValue(player_) == 21) stand();  // natural blackjack
}

void Blackjack::stand() {
  while (handValue(dealer_) < 17) dealer_.push_back(draw());
  int pv = handValue(player_), dv = handValue(dealer_);
  if (pv > 21) result_ = 2;
  else if (player_.size() == 2 && pv == 21) result_ = 4;
  else if (dv > 21 || pv > dv) result_ = 1;
  else if (pv < dv) result_ = 2;
  else result_ = 3;
  state_ = 1;
}

bool Blackjack::onButton(Btn b) {
  if (state_ == 1) {
    if (b == Btn::Confirm) { deal(); return true; }
    return false;  // Back -> home
  }
  switch (b) {
    case Btn::Up:
    case Btn::Confirm:  // Hit
      player_.push_back(draw());
      if (handValue(player_) > 21) { result_ = 2; state_ = 1; }
      return true;
    case Btn::Down:     // Stand
      stand();
      return true;
    default: return false;  // Back -> home
  }
}

static String cardName(int r) {
  if (r == 1) return "A";
  if (r == 11) return "J";
  if (r == 12) return "Q";
  if (r == 13) return "K";
  return String(r);
}

static void drawHand(GFXcanvas1& g, const std::vector<int>& h, int y,
                     bool hideSecond) {
  const int cw = 66, ch = 92, gap = 12;
  int total = (int)h.size();
  int x0 = (SCREEN_W - (total * cw + (total - 1) * gap)) / 2;
  for (int i = 0; i < total; ++i) {
    int x = x0 + i * (cw + gap);
    g.drawRoundRect(x, y, cw, ch, 6, UI_BLACK);
    g.drawRoundRect(x + 1, y + 1, cw - 2, ch - 2, 6, UI_BLACK);
    String s = (hideSecond && i == 1) ? "?" : cardName(h[i]);
    duet::centerText(g, x + cw / 2, y + ch / 2 + 8, FONT_LARGE, s, UI_BLACK);
  }
}

void Blackjack::render(DuetDisplay& d) {
  auto& g = d.gfx();
  const char* h = state_ == 0 ? "Blackjack"
                : result_ == 1 ? "Blackjack  -  You win!"
                : result_ == 4 ? "Blackjack  -  Blackjack!"
                : result_ == 3 ? "Blackjack  -  Push"
                               : "Blackjack  -  Dealer wins";
  duet::header(g, h);

  bool hide = (state_ == 0);
  duet::text(g, UI_MARGIN, UI_HEADER_H + 30, FONT_MED,
             String("Dealer: ") + (hide ? String("?") : String(handValue(dealer_))),
             UI_BLACK);
  drawHand(g, dealer_, UI_HEADER_H + 44, hide);

  duet::text(g, UI_MARGIN, 320, FONT_MED,
             String("You: ") + handValue(player_), UI_BLACK);
  drawHand(g, player_, 334, false);

  duet::footerHint(g, state_ == 0
                          ? "Up/Confirm: Hit    Down: Stand    Back: home"
                          : "Confirm: new hand    Back: home");
}
