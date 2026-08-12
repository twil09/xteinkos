#include "ConnectFourActivity.h"

#include <Arduino.h>
#include <I18n.h>

#include "HalDisplay.h"
#include "components/UITheme.h"
#include "fontIds.h"

void ConnectFourActivity::reset() {
  for (int r = 0; r < ROWS; ++r)
    for (int c = 0; c < COLS; ++c) board[r][c] = 0;
  cursorCol = COLS / 2;
  over = false;
  winner = 0;
  draw = false;
}

void ConnectFourActivity::onEnter() {
  Activity::onEnter();
  reset();
  requestUpdate(true);
}

int ConnectFourActivity::dropRow(int c) const {
  for (int r = ROWS - 1; r >= 0; --r)
    if (board[r][c] == 0) return r;
  return -1;
}

bool ConnectFourActivity::place(int c, uint8_t who) {
  const int r = dropRow(c);
  if (r < 0) return false;
  board[r][c] = who;
  return true;
}

bool ConnectFourActivity::boardFull() const {
  for (int c = 0; c < COLS; ++c)
    if (dropRow(c) >= 0) return false;
  return true;
}

bool ConnectFourActivity::wins(uint8_t who) const {
  for (int r = 0; r < ROWS; ++r)
    for (int c = 0; c < COLS; ++c) {
      if (board[r][c] != who) continue;
      if (c + 3 < COLS && board[r][c + 1] == who && board[r][c + 2] == who && board[r][c + 3] == who) return true;
      if (r + 3 < ROWS && board[r + 1][c] == who && board[r + 2][c] == who && board[r + 3][c] == who) return true;
      if (r + 3 < ROWS && c + 3 < COLS && board[r + 1][c + 1] == who && board[r + 2][c + 2] == who &&
          board[r + 3][c + 3] == who)
        return true;
      if (r - 3 >= 0 && c + 3 < COLS && board[r - 1][c + 1] == who && board[r - 2][c + 2] == who &&
          board[r - 3][c + 3] == who)
        return true;
    }
  return false;
}

void ConnectFourActivity::aiMove() {
  // 1) win if possible
  for (int c = 0; c < COLS; ++c) {
    int r = dropRow(c);
    if (r < 0) continue;
    board[r][c] = 2;
    if (wins(2)) return;  // leave it played
    board[r][c] = 0;
  }
  // 2) block the player
  for (int c = 0; c < COLS; ++c) {
    int r = dropRow(c);
    if (r < 0) continue;
    board[r][c] = 1;
    bool threat = wins(1);
    board[r][c] = 0;
    if (threat) { place(c, 2); return; }
  }
  // 3) center-first fallback
  const int order[COLS] = {3, 2, 4, 1, 5, 0, 6};
  for (int i = 0; i < COLS; ++i)
    if (place(order[i], 2)) return;
}

void ConnectFourActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) { finish(); return; }
  if (over) {
    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) { reset(); requestUpdate(); }
    return;
  }
  if (mappedInput.wasPressed(MappedInputManager::Button::Left)) {
    cursorCol = (cursorCol + COLS - 1) % COLS; requestUpdate();
  } else if (mappedInput.wasPressed(MappedInputManager::Button::Right)) {
    cursorCol = (cursorCol + 1) % COLS; requestUpdate();
  } else if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    if (!place(cursorCol, 1)) return;
    if (wins(1)) { over = true; winner = 1; requestUpdate(); return; }
    if (boardFull()) { over = true; draw = true; requestUpdate(); return; }
    aiMove();
    if (wins(2)) { over = true; winner = 2; }
    else if (boardFull()) { over = true; draw = true; }
    requestUpdate();
  }
}

void ConnectFourActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();

  renderer.clearScreen();
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.homeTopPadding}, "Connect Four");

  const int headerBottom = metrics.headerHeight + metrics.homeTopPadding;
  const int availH = pageHeight - headerBottom - metrics.buttonHintsHeight - 70;
  int cell = (pageWidth * 96 / 100) / COLS;
  if (cell > availH / (ROWS + 1)) cell = availH / (ROWS + 1);  // +1 row for the drop indicator
  const int boardW = cell * COLS, boardH = cell * ROWS;
  const int ox = (pageWidth - boardW) / 2;
  const int oy = headerBottom + cell + 8;  // leave a row above for the cursor piece

  // drop indicator above the chosen column
  if (!over) {
    const int dx = ox + cursorCol * cell;
    renderer.fillRoundedRect(dx + 5, headerBottom + 6, cell - 10, cell - 10, (cell - 10) / 2, Color::Black);
  }

  for (int r = 0; r < ROWS; ++r)
    for (int c = 0; c < COLS; ++c) {
      const int x = ox + c * cell, y = oy + r * cell;
      const int d = cell - 8, ix = x + 4, iy = y + 4, rad = d / 2;
      if (board[r][c] == 1)
        renderer.fillRoundedRect(ix, iy, d, d, rad, Color::Black);           // player disc
      else if (board[r][c] == 2)
        renderer.drawRoundedRect(ix, iy, d, d, 3, rad, true);                // AI ring
      else
        renderer.drawRoundedRect(ix, iy, d, d, 1, rad, true);                // empty hole
    }

  const char* status = "Your turn";
  if (over) status = draw ? "Draw!" : (winner == 1 ? "You win!" : "Vix wins!");
  renderer.drawCenteredText(UI_12_FONT_ID, oy + boardH + 28, status, true, EpdFontFamily::BOLD);

  const auto labels = over ? mappedInput.mapLabels(tr(STR_BACK), "New game", "", "")
                           : mappedInput.mapLabels(tr(STR_BACK), "Drop", "", "");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer(HalDisplay::RefreshMode::FAST_REFRESH);
}
