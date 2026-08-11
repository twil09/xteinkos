#include "MinesweeperActivity.h"

#include <Arduino.h>
#include <I18n.h>

#include <cstdio>

#include "HalDisplay.h"
#include "components/UITheme.h"
#include "fontIds.h"

void MinesweeperActivity::reset() {
  for (int r = 0; r < ROWS; ++r)
    for (int c = 0; c < COLS; ++c) { mine[r][c] = false; shown[r][c] = false; }
  curR = curC = 0;
  started = false;
  over = won = false;
}

void MinesweeperActivity::onEnter() {
  Activity::onEnter();
  reset();
  requestUpdate(true);
}

// Place mines avoiding the first-revealed cell and its neighbours.
void MinesweeperActivity::placeMines(int safeR, int safeC) {
  int placed = 0;
  while (placed < MINES) {
    int r = random(ROWS), c = random(COLS);
    if (mine[r][c]) continue;
    if (r >= safeR - 1 && r <= safeR + 1 && c >= safeC - 1 && c <= safeC + 1) continue;
    mine[r][c] = true;
    ++placed;
  }
  started = true;
}

int MinesweeperActivity::adjacent(int r, int c) const {
  int n = 0;
  for (int dr = -1; dr <= 1; ++dr)
    for (int dc = -1; dc <= 1; ++dc) {
      if (!dr && !dc) continue;
      int nr = r + dr, nc = c + dc;
      if (nr >= 0 && nr < ROWS && nc >= 0 && nc < COLS && mine[nr][nc]) ++n;
    }
  return n;
}

void MinesweeperActivity::reveal(int r, int c) {
  if (r < 0 || r >= ROWS || c < 0 || c >= COLS || shown[r][c]) return;
  shown[r][c] = true;
  if (mine[r][c]) { over = true; won = false; return; }
  if (adjacent(r, c) == 0)
    for (int dr = -1; dr <= 1; ++dr)
      for (int dc = -1; dc <= 1; ++dc)
        if (dr || dc) reveal(r + dr, c + dc);
}

bool MinesweeperActivity::checkWin() const {
  for (int r = 0; r < ROWS; ++r)
    for (int c = 0; c < COLS; ++c)
      if (!mine[r][c] && !shown[r][c]) return false;
  return true;
}

void MinesweeperActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) { finish(); return; }
  if (over) {
    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) { reset(); requestUpdate(); }
    return;
  }
  if (mappedInput.wasPressed(MappedInputManager::Button::Left)) { curC = (curC + COLS - 1) % COLS; requestUpdate(); }
  else if (mappedInput.wasPressed(MappedInputManager::Button::Right)) { curC = (curC + 1) % COLS; requestUpdate(); }
  else if (mappedInput.wasPressed(MappedInputManager::Button::Up)) { curR = (curR + ROWS - 1) % ROWS; requestUpdate(); }
  else if (mappedInput.wasPressed(MappedInputManager::Button::Down)) { curR = (curR + 1) % ROWS; requestUpdate(); }
  else if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    if (!started) placeMines(curR, curC);
    reveal(curR, curC);
    if (!over && checkWin()) { over = true; won = true; }
    requestUpdate();
  }
}

void MinesweeperActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();

  renderer.clearScreen();
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.homeTopPadding}, "Minesweeper");

  const int headerBottom = metrics.headerHeight + metrics.homeTopPadding;
  const int availH = pageHeight - headerBottom - metrics.buttonHintsHeight - 50;
  int cell = (pageWidth * 92 / 100) / COLS;
  if (cell > availH / ROWS) cell = availH / ROWS;
  const int boardW = cell * COLS, boardH = cell * ROWS;
  const int ox = (pageWidth - boardW) / 2, oy = headerBottom + 16;

  char buf[4];
  for (int r = 0; r < ROWS; ++r)
    for (int c = 0; c < COLS; ++c) {
      const int x = ox + c * cell, y = oy + r * cell;
      if (shown[r][c]) {
        if (mine[r][c])
          renderer.fillRoundedRect(x + 3, y + 3, cell - 6, cell - 6, 4, Color::Black);
        else {
          renderer.drawRoundedRect(x + 1, y + 1, cell - 2, cell - 2, 1, 3, true);
          int a = adjacent(r, c);
          if (a > 0) {
            snprintf(buf, sizeof(buf), "%d", a);
            int tw = renderer.getTextWidth(UI_12_FONT_ID, buf, EpdFontFamily::BOLD);
            renderer.drawText(UI_12_FONT_ID, x + (cell - tw) / 2, y + cell * 66 / 100, buf, true,
                              EpdFontFamily::BOLD);
          }
        }
      } else {
        renderer.fillRoundedRect(x + 2, y + 2, cell - 4, cell - 4, 3, Color::LightGray);
        renderer.drawRoundedRect(x + 2, y + 2, cell - 4, cell - 4, 1, 3, true);
      }
      if (!over && r == curR && c == curC) renderer.drawRect(x, y, cell, cell, 2, true);
    }

  const char* status = !over ? "Find the safe cells" : (won ? "Cleared!" : "Boom!");
  renderer.drawCenteredText(UI_12_FONT_ID, oy + boardH + 26, status, true, EpdFontFamily::BOLD);

  const auto labels = over ? mappedInput.mapLabels(tr(STR_BACK), "New game", "", "")
                           : mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer(HalDisplay::RefreshMode::FAST_REFRESH);
}
