#include "TicTacToeActivity.h"

#include <I18n.h>

#include "HalDisplay.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
constexpr int kLines[8][3] = {{0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {0, 3, 6},
                              {1, 4, 7}, {2, 5, 8}, {0, 4, 8}, {2, 4, 6}};
}  // namespace

void TicTacToeActivity::reset() {
  for (int i = 0; i < 9; ++i) board[i] = 0;
  cursor = 4;
  winner = 0;
  over = false;
  draw = false;
}

void TicTacToeActivity::onEnter() {
  Activity::onEnter();
  reset();
  requestUpdate(true);
}

uint8_t TicTacToeActivity::lineWinner() const {
  for (auto& l : kLines) {
    uint8_t a = board[l[0]];
    if (a != 0 && a == board[l[1]] && a == board[l[2]]) return a;
  }
  return 0;
}

bool TicTacToeActivity::boardFull() const {
  for (int i = 0; i < 9; ++i)
    if (board[i] == 0) return false;
  return true;
}

// The empty cell that would complete a line for `who`, or -1.
int TicTacToeActivity::findWinningMove(uint8_t who) const {
  for (auto& l : kLines) {
    int empty = -1, mine = 0;
    for (int k = 0; k < 3; ++k) {
      if (board[l[k]] == who)
        ++mine;
      else if (board[l[k]] == 0)
        empty = l[k];
      else {
        mine = -9;
        break;
      }
    }
    if (mine == 2 && empty >= 0) return empty;
  }
  return -1;
}

void TicTacToeActivity::aiMove() {
  int move = findWinningMove(2);            // win if possible
  if (move < 0) move = findWinningMove(1);  // else block the player
  if (move < 0 && board[4] == 0) move = 4;  // else center
  if (move < 0) {
    const int corners[4] = {0, 2, 6, 8};
    for (int c : corners)
      if (board[c] == 0) { move = c; break; }
  }
  if (move < 0)
    for (int i = 0; i < 9; ++i)
      if (board[i] == 0) { move = i; break; }
  if (move >= 0) board[move] = 2;
}

void TicTacToeActivity::placeAndRespond(int cell) {
  if (board[cell] != 0) return;
  board[cell] = 1;
  winner = lineWinner();
  if (winner) { over = true; return; }
  if (boardFull()) { over = true; draw = true; return; }
  aiMove();
  winner = lineWinner();
  if (winner) { over = true; return; }
  if (boardFull()) { over = true; draw = true; }
}

void TicTacToeActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    finish();
    return;
  }
  if (over) {
    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
      reset();
      requestUpdate();
    }
    return;
  }
  if (mappedInput.wasPressed(MappedInputManager::Button::Left)) {
    cursor = (cursor + 8) % 9;
    requestUpdate();
  } else if (mappedInput.wasPressed(MappedInputManager::Button::Right)) {
    cursor = (cursor + 1) % 9;
    requestUpdate();
  } else if (mappedInput.wasPressed(MappedInputManager::Button::Up)) {
    cursor = (cursor + 6) % 9;
    requestUpdate();
  } else if (mappedInput.wasPressed(MappedInputManager::Button::Down)) {
    cursor = (cursor + 3) % 9;
    requestUpdate();
  } else if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    placeAndRespond(cursor);
    requestUpdate();
  }
}

void TicTacToeActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();

  renderer.clearScreen();
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.homeTopPadding}, "Tic-Tac-Toe");

  const int headerBottom = metrics.headerHeight + metrics.homeTopPadding;
  const int avail = pageHeight - headerBottom - metrics.buttonHintsHeight - 60;  // leave room for status
  int boardSize = (pageWidth < avail ? pageWidth : avail) * 82 / 100;
  const int cell = boardSize / 3;
  boardSize = cell * 3;
  const int bx = (pageWidth - boardSize) / 2;
  const int by = headerBottom + 20;

  // grid
  for (int i = 1; i < 3; ++i) {
    renderer.drawLine(bx + i * cell, by, bx + i * cell, by + boardSize, 3, true);
    renderer.drawLine(bx, by + i * cell, bx + boardSize, by + i * cell, 3, true);
  }

  // marks
  const int inset = cell * 22 / 100;
  for (int idx = 0; idx < 9; ++idx) {
    const int r = idx / 3, c = idx % 3;
    const int x0 = bx + c * cell, y0 = by + r * cell;
    if (board[idx] == 1) {  // X
      renderer.drawLine(x0 + inset, y0 + inset, x0 + cell - inset, y0 + cell - inset, 5, true);
      renderer.drawLine(x0 + cell - inset, y0 + inset, x0 + inset, y0 + cell - inset, 5, true);
    } else if (board[idx] == 2) {  // O (rounded square ~ circle)
      const int d = cell - 2 * inset;
      renderer.drawRoundedRect(x0 + inset, y0 + inset, d, d, 5, d / 2, true);
    }
  }

  // cursor highlight while playing
  if (!over) {
    const int r = cursor / 3, c = cursor % 3;
    renderer.drawRect(bx + c * cell + 4, by + r * cell + 4, cell - 8, cell - 8, 2, true);
  }

  // status line
  const char* status = "Your turn";
  if (over) status = draw ? "Draw!" : (winner == 1 ? "You win!" : "Vix wins!");
  renderer.drawCenteredText(UI_12_FONT_ID, by + boardSize + 34, status, true, EpdFontFamily::BOLD);

  const auto labels = over
                          ? mappedInput.mapLabels(tr(STR_BACK), "New game", "", "")
                          : mappedInput.mapLabels(tr(STR_BACK), "Place", tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer(HalDisplay::RefreshMode::FAST_REFRESH);
}
