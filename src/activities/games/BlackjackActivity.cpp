#include "BlackjackActivity.h"

#include <Arduino.h>
#include <I18n.h>

#include <cstdio>

#include "HalDisplay.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
const char* rankStr(uint8_t card) {
  static const char* const kNames[13] = {"A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K"};
  return kNames[card % 13];
}
const char* suitStr(uint8_t card) {
  static const char* const kSuits[4] = {"S", "H", "D", "C"};
  return kSuits[card / 13];
}
}  // namespace

int BlackjackActivity::handValue(const uint8_t* cards, int n) const {
  int total = 0, aces = 0;
  for (int i = 0; i < n; ++i) {
    const int rank = cards[i] % 13;
    total += (rank == 0) ? 11 : (rank >= 9 ? 10 : rank + 1);
    if (rank == 0) ++aces;
  }
  while (total > 21 && aces > 0) {  // soften aces from 11 to 1
    total -= 10;
    --aces;
  }
  return total;
}

void BlackjackActivity::newHand() {
  for (int i = 0; i < 52; ++i) deck[i] = (uint8_t)i;
  for (int i = 51; i > 0; --i) {
    const int j = random(i + 1);
    const uint8_t t = deck[i];
    deck[i] = deck[j];
    deck[j] = t;
  }
  deckPos = 0;
  playerCount = dealerCount = 0;
  player[playerCount++] = deck[deckPos++];
  dealer[dealerCount++] = deck[deckPos++];
  player[playerCount++] = deck[deckPos++];
  dealer[dealerCount++] = deck[deckPos++];
  phase = PLAYER;
  outcome = NONE;
  revealed = false;
  if (handValue(player, playerCount) == 21) stand();  // natural: settle immediately
}

void BlackjackActivity::hit() {
  if (deckPos < 52) player[playerCount++] = deck[deckPos++];
  if (handValue(player, playerCount) > 21) {
    revealed = true;
    outcome = LOSE;
    phase = RESULT;
  }
}

void BlackjackActivity::stand() {
  revealed = true;
  while (handValue(dealer, dealerCount) < 17 && deckPos < 52) dealer[dealerCount++] = deck[deckPos++];
  settle();
  phase = RESULT;
}

void BlackjackActivity::settle() {
  const int p = handValue(player, playerCount), d = handValue(dealer, dealerCount);
  if (p > 21) outcome = LOSE;
  else if (d > 21) outcome = WIN;
  else if (p > d) outcome = WIN;
  else if (p < d) outcome = LOSE;
  else outcome = PUSH;
}

void BlackjackActivity::onEnter() {
  Activity::onEnter();
  randomSeed(micros());
  newHand();
  requestUpdate(true);
}

void BlackjackActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) { finish(); return; }
  if (phase == RESULT) {
    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) { newHand(); requestUpdate(); }
    return;
  }
  if (mappedInput.wasPressed(MappedInputManager::Button::Up)) { hit(); requestUpdate(); }
  else if (mappedInput.wasPressed(MappedInputManager::Button::Down)) { stand(); requestUpdate(); }
}

void BlackjackActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();

  renderer.clearScreen();
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.homeTopPadding}, "Blackjack");

  const int headerBottom = metrics.headerHeight + metrics.homeTopPadding;

  int maxCards = playerCount > dealerCount ? playerCount : dealerCount;
  if (maxCards < 5) maxCards = 5;
  int cardW = (pageWidth - 40) / maxCards - 6;
  if (cardW > 50) cardW = 50;
  const int cardH = cardW * 3 / 2, gap = 6;

  auto drawCard = [&](int x, int y, uint8_t card, bool faceDown) {
    if (faceDown) {
      renderer.fillRoundedRect(x + 2, y + 2, cardW - 4, cardH - 4, 5, Color::LightGray);
      renderer.drawRoundedRect(x, y, cardW, cardH, 2, 6, true);
      return;
    }
    renderer.drawRoundedRect(x, y, cardW, cardH, 2, 6, true);
    const int rw = renderer.getTextWidth(NOTOSANS_16_FONT_ID, rankStr(card), EpdFontFamily::BOLD);
    renderer.drawText(NOTOSANS_16_FONT_ID, x + (cardW - rw) / 2, y + cardH * 46 / 100, rankStr(card), true,
                      EpdFontFamily::BOLD);
    const int sw = renderer.getTextWidth(UI_10_FONT_ID, suitStr(card));
    renderer.drawText(UI_10_FONT_ID, x + (cardW - sw) / 2, y + cardH * 82 / 100, suitStr(card), true);
  };

  auto drawRow = [&](int y, const uint8_t* cards, int n, bool hideHole) {
    for (int i = 0; i < n; ++i)
      drawCard(20 + i * (cardW + gap), y, cards[i], hideHole && i == 1);
  };

  char buf[24];
  const int dealerY = headerBottom + 24;
  const int dealerVal = handValue(dealer, dealerCount);
  if (revealed) snprintf(buf, sizeof(buf), "Dealer: %d", dealerVal);
  else snprintf(buf, sizeof(buf), "Dealer: ?");
  renderer.drawText(UI_12_FONT_ID, 20, dealerY, buf, true, EpdFontFamily::BOLD);
  drawRow(dealerY + 8, dealer, dealerCount, !revealed);

  const int playerY = dealerY + 8 + cardH + 30;
  snprintf(buf, sizeof(buf), "You: %d", handValue(player, playerCount));
  renderer.drawText(UI_12_FONT_ID, 20, playerY, buf, true, EpdFontFamily::BOLD);
  drawRow(playerY + 8, player, playerCount, false);

  const char* status = "Hit or stand?";
  if (phase == RESULT) {
    if (outcome == WIN) status = handValue(dealer, dealerCount) > 21 ? "Dealer bust - you win!" : "You win!";
    else if (outcome == LOSE) status = handValue(player, playerCount) > 21 ? "Bust!" : "Dealer wins";
    else status = "Push";
  }
  renderer.drawCenteredText(UI_12_FONT_ID, playerY + 8 + cardH + 28, status, true, EpdFontFamily::BOLD);

  const auto labels = phase == RESULT
                          ? mappedInput.mapLabels(tr(STR_BACK), "Deal", "", "")
                          : mappedInput.mapLabels(tr(STR_BACK), "", "Hit", "Stand");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer(HalDisplay::RefreshMode::FAST_REFRESH);
}
