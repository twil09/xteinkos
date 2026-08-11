#include "Game2048Activity.h"

#include <Arduino.h>
#include <I18n.h>

#include <cstdio>

#include "HalDisplay.h"
#include "components/UITheme.h"
#include "fontIds.h"

void Game2048Activity::reset() {
  for (int r = 0; r < N; ++r)
    for (int c = 0; c < N; ++c) board[r][c] = 0;
  score = 0;
  over = false;
  spawn();
  spawn();
}

void Game2048Activity::onEnter() {
  Activity::onEnter();
  reset();
  requestUpdate(true);
}

void Game2048Activity::spawn() {
  int empties[N * N][2];
  int n = 0;
  for (int r = 0; r < N; ++r)
    for (int c = 0; c < N; ++c)
      if (board[r][c] == 0) { empties[n][0] = r; empties[n][1] = c; ++n; }
  if (n == 0) return;
  int idx = random(n);
  board[empties[idx][0]][empties[idx][1]] = (random(10) == 0) ? 4 : 2;
}

// Compress + merge one line of 4 (in-place); returns true if it changed.
static bool slideLine(int* line, long& score) {
  int tmp[4] = {0, 0, 0, 0};
  int t = 0;
  for (int i = 0; i < 4; ++i)
    if (line[i]) tmp[t++] = line[i];
  int out[4] = {0, 0, 0, 0};
  int o = 0;
  for (int i = 0; i < 4; ++i) {
    if (tmp[i] == 0) break;
    if (i + 1 < 4 && tmp[i] == tmp[i + 1]) {
      out[o++] = tmp[i] * 2;
      score += tmp[i] * 2;
      ++i;  // consume the merged partner
    } else {
      out[o++] = tmp[i];
    }
  }
  bool changed = false;
  for (int i = 0; i < 4; ++i) {
    if (line[i] != out[i]) changed = true;
    line[i] = out[i];
  }
  return changed;
}

bool Game2048Activity::slide(int dir) {
  bool changed = false;
  if (dir == 0 || dir == 1) {  // horizontal
    for (int r = 0; r < N; ++r) {
      int line[4];
      for (int c = 0; c < N; ++c) line[c] = dir == 0 ? board[r][c] : board[r][N - 1 - c];
      if (slideLine(line, score)) changed = true;
      for (int c = 0; c < N; ++c) (dir == 0 ? board[r][c] : board[r][N - 1 - c]) = line[c];
    }
  } else {  // vertical
    for (int c = 0; c < N; ++c) {
      int line[4];
      for (int r = 0; r < N; ++r) line[r] = dir == 2 ? board[r][c] : board[N - 1 - r][c];
      if (slideLine(line, score)) changed = true;
      for (int r = 0; r < N; ++r) (dir == 2 ? board[r][c] : board[N - 1 - r][c]) = line[r];
    }
  }
  return changed;
}

bool Game2048Activity::movesAvailable() const {
  for (int r = 0; r < N; ++r)
    for (int c = 0; c < N; ++c) {
      if (board[r][c] == 0) return true;
      if (c + 1 < N && board[r][c] == board[r][c + 1]) return true;
      if (r + 1 < N && board[r][c] == board[r + 1][c]) return true;
    }
  return false;
}

void Game2048Activity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) { finish(); return; }
  if (over) {
    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) { reset(); requestUpdate(); }
    return;
  }
  int dir = -1;
  if (mappedInput.wasPressed(MappedInputManager::Button::Left)) dir = 0;
  else if (mappedInput.wasPressed(MappedInputManager::Button::Right)) dir = 1;
  else if (mappedInput.wasPressed(MappedInputManager::Button::Up)) dir = 2;
  else if (mappedInput.wasPressed(MappedInputManager::Button::Down)) dir = 3;
  if (dir < 0) return;
  if (slide(dir)) {
    spawn();
    if (!movesAvailable()) over = true;
    requestUpdate();
  }
}

void Game2048Activity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();

  renderer.clearScreen();
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.homeTopPadding}, "2048");

  char buf[32];
  snprintf(buf, sizeof(buf), "Score: %ld", score);
  const int headerBottom = metrics.headerHeight + metrics.homeTopPadding;
  renderer.drawCenteredText(UI_12_FONT_ID, headerBottom + 6, buf, true, EpdFontFamily::BOLD);

  const int avail = pageHeight - headerBottom - metrics.buttonHintsHeight - 70;
  int boardSize = (pageWidth < avail ? pageWidth : avail) * 88 / 100;
  const int cell = boardSize / N;
  boardSize = cell * N;
  const int bx = (pageWidth - boardSize) / 2;
  const int by = headerBottom + 36;

  for (int r = 0; r < N; ++r)
    for (int c = 0; c < N; ++c) {
      const int x = bx + c * cell, y = by + r * cell;
      renderer.drawRoundedRect(x + 2, y + 2, cell - 4, cell - 4, 2, 6, true);
      if (board[r][c]) {
        snprintf(buf, sizeof(buf), "%d", board[r][c]);
        const int tw = renderer.getTextWidth(NOTOSANS_16_FONT_ID, buf);
        renderer.drawText(NOTOSANS_16_FONT_ID, x + (cell - tw) / 2, y + cell * 62 / 100, buf, true,
                          EpdFontFamily::BOLD);
      }
    }

  if (over)
    renderer.drawCenteredText(UI_12_FONT_ID, by + boardSize + 30, "Game over", true, EpdFontFamily::BOLD);

  const auto labels = over ? mappedInput.mapLabels(tr(STR_BACK), "New game", "", "")
                           : mappedInput.mapLabels(tr(STR_BACK), "", tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer(HalDisplay::RefreshMode::FAST_REFRESH);
}
