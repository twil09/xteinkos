#include "ReversiActivity.h"

#include <I18n.h>

#include <cstdio>

#include "HalDisplay.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
constexpr int DR[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
constexpr int DC[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
}  // namespace

void ReversiActivity::reset() {
  for (int r = 0; r < N; ++r)
    for (int c = 0; c < N; ++c) board[r][c] = 0;
  board[3][3] = 2; board[3][4] = 1;
  board[4][3] = 1; board[4][4] = 2;
  curR = curC = 3;
  playerTurn = true;
  over = false;
}

void ReversiActivity::onEnter() {
  Activity::onEnter();
  reset();
  requestUpdate(true);
}

// Number of discs flipped by placing `who` at (r,c); if apply, perform the move.
int ReversiActivity::gain(uint8_t who, int r, int c, bool apply) {
  if (board[r][c] != 0) return 0;
  const uint8_t other = 3 - who;
  int total = 0;
  for (int d = 0; d < 8; ++d) {
    int rr = r + DR[d], cc = c + DC[d], run = 0;
    while (rr >= 0 && rr < N && cc >= 0 && cc < N && board[rr][cc] == other) {
      rr += DR[d]; cc += DC[d]; ++run;
    }
    if (run > 0 && rr >= 0 && rr < N && cc >= 0 && cc < N && board[rr][cc] == who) {
      total += run;
      if (apply)
        for (int k = 1; k <= run; ++k) board[r + DR[d] * k][c + DC[d] * k] = who;
    }
  }
  if (apply && total > 0) board[r][c] = who;
  return total;
}

bool ReversiActivity::hasMove(uint8_t who) const {
  for (int r = 0; r < N; ++r)
    for (int c = 0; c < N; ++c)
      if (board[r][c] == 0 && const_cast<ReversiActivity*>(this)->gain(who, r, c, false) > 0) return true;
  return false;
}

int ReversiActivity::count(uint8_t who) const {
  int n = 0;
  for (int r = 0; r < N; ++r)
    for (int c = 0; c < N; ++c)
      if (board[r][c] == who) ++n;
  return n;
}

void ReversiActivity::aiTurns() {
  while (!over) {
    // AI (2) to move
    int bestR = -1, bestC = -1, best = 0;
    for (int r = 0; r < N; ++r)
      for (int c = 0; c < N; ++c) {
        int g = gain(2, r, c, false);
        if (g > best) { best = g; bestR = r; bestC = c; }
      }
    if (bestR >= 0) gain(2, bestR, bestC, true);
    else if (!hasMove(1)) { over = true; break; }  // AI passes and player can't move
    if (hasMove(1)) break;                           // hand control to player
    if (!hasMove(2)) { over = true; break; }         // player passes, AI can't -> over
  }
}

void ReversiActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) { finish(); return; }
  if (over) {
    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) { reset(); requestUpdate(); }
    return;
  }
  if (mappedInput.wasPressed(MappedInputManager::Button::Left)) { curC = (curC + N - 1) % N; requestUpdate(); }
  else if (mappedInput.wasPressed(MappedInputManager::Button::Right)) { curC = (curC + 1) % N; requestUpdate(); }
  else if (mappedInput.wasPressed(MappedInputManager::Button::Up)) { curR = (curR + N - 1) % N; requestUpdate(); }
  else if (mappedInput.wasPressed(MappedInputManager::Button::Down)) { curR = (curR + 1) % N; requestUpdate(); }
  else if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    if (gain(1, curR, curC, true) > 0) {  // legal player move
      aiTurns();
      requestUpdate();
    }
  }
}

void ReversiActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();

  renderer.clearScreen();
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.homeTopPadding}, "Reversi");

  const int headerBottom = metrics.headerHeight + metrics.homeTopPadding;
  const int availH = pageHeight - headerBottom - metrics.buttonHintsHeight - 60;
  int boardSize = (pageWidth < availH ? pageWidth : availH) * 94 / 100;
  const int cell = boardSize / N;
  boardSize = cell * N;
  const int ox = (pageWidth - boardSize) / 2;
  const int oy = headerBottom + 16;

  for (int i = 0; i <= N; ++i) {
    renderer.drawLine(ox + i * cell, oy, ox + i * cell, oy + boardSize, true);
    renderer.drawLine(ox, oy + i * cell, ox + boardSize, oy + i * cell, true);
  }
  for (int r = 0; r < N; ++r)
    for (int c = 0; c < N; ++c) {
      const int x = ox + c * cell, y = oy + r * cell, d = cell - 6, rad = (cell - 6) / 2;
      if (board[r][c] == 1) renderer.fillRoundedRect(x + 3, y + 3, d, d, rad, Color::Black);
      else if (board[r][c] == 2) renderer.drawRoundedRect(x + 3, y + 3, d, d, 3, rad, true);
      else if (!over && gain(1, r, c, false) > 0)  // hint: a legal spot
        renderer.fillRoundedRect(x + cell / 2 - 2, y + cell / 2 - 2, 4, 4, 2, Color::Black);
    }
  if (!over) renderer.drawRect(ox + curC * cell + 1, oy + curR * cell + 1, cell - 2, cell - 2, 2, true);

  char buf[40];
  const int you = count(1), vix = count(2);
  if (over)
    snprintf(buf, sizeof(buf), "%s  (You %d - %d Vix)", you > vix ? "You win!" : vix > you ? "Vix wins!" : "Draw",
             you, vix);
  else
    snprintf(buf, sizeof(buf), "You %d   Vix %d", you, vix);
  renderer.drawCenteredText(UI_12_FONT_ID, oy + boardSize + 26, buf, true, EpdFontFamily::BOLD);

  const auto labels = over ? mappedInput.mapLabels(tr(STR_BACK), "New game", "", "")
                           : mappedInput.mapLabels(tr(STR_BACK), "Place", tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer(HalDisplay::RefreshMode::FAST_REFRESH);
}
