#pragma once
#include <cstdint>

#include "activities/Activity.h"

// Single-hand Blackjack vs the dealer. Up = Hit, Down = Stand, Confirm = deal a
// new hand. Dealer hits below 17 and its hole card stays hidden until the stand.
class BlackjackActivity : public Activity {
 public:
  BlackjackActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Blackjack", renderer, mappedInput) {}
  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  enum Phase : uint8_t { PLAYER, RESULT };
  enum Outcome : uint8_t { NONE, WIN, LOSE, PUSH };

  uint8_t deck[52] = {0};  // cards 0..51, rank = card % 13, suit = card / 13
  int deckPos = 0;
  uint8_t player[16] = {0};
  uint8_t dealer[16] = {0};
  int playerCount = 0, dealerCount = 0;
  Phase phase = PLAYER;
  Outcome outcome = NONE;
  bool revealed = false;  // dealer hole card face-up

  void newHand();
  int handValue(const uint8_t* cards, int n) const;
  void hit();
  void stand();
  void settle();
};
