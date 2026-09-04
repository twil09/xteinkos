#include "SudokuActivity.h"

#include <Arduino.h>
#include <I18n.h>

#include <cstdio>

#include "HalDisplay.h"
#include "components/UITheme.h"
#include "fontIds.h"

bool SudokuActivity::canPlace(int pos, uint8_t v) const {
  const int r = pos / 9, c = pos % 9;
  for (int i = 0; i < 9; ++i) {
    if (grid[r * 9 + i] == v) return false;
    if (grid[i * 9 + c] == v) return false;
  }
  const int br = (r / 3) * 3, bc = (c / 3) * 3;
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      if (grid[(br + i) * 9 + (bc + j)] == v) return false;
  return true;
}

// Fill from an empty grid; a random starting digit per cell varies the puzzle
// while keeping only a 1-byte offset per stack frame (81-deep worst case).
bool SudokuActivity::fillGrid(int pos) {
  if (pos == 81) return true;
  if (grid[pos] != 0) return fillGrid(pos + 1);
  const uint8_t start = (uint8_t)random(9);
  for (uint8_t k = 0; k < 9; ++k) {
    const uint8_t v = (uint8_t)((start + k) % 9) + 1;
    if (canPlace(pos, v)) {
      grid[pos] = v;
      if (fillGrid(pos + 1)) return true;
      grid[pos] = 0;
    }
  }
  return false;
}

void SudokuActivity::generate() {
  for (int i = 0; i < 81; ++i) grid[i] = 0;
  fillGrid(0);
  for (int i = 0; i < 81; ++i) solution[i] = grid[i];

  uint8_t order[81];
  for (int i = 0; i < 81; ++i) order[i] = (uint8_t)i;
  for (int i = 80; i > 0; --i) {
    const int j = random(i + 1);
    const uint8_t t = order[i];
    order[i] = order[j];
    order[j] = t;
  }
  for (int i = 0; i < 81; ++i) fixed[i] = true;
  for (int i = 0; i < HOLES; ++i) {
    const int p = order[i];
    grid[p] = 0;
    fixed[p] = false;
  }
  curR = curC = 0;
  won = false;
}

bool SudokuActivity::solved() const {
  for (int i = 0; i < 81; ++i)
    if (grid[i] == 0) return false;
  for (int u = 0; u < 9; ++u) {
    uint16_t rowMask = 0, colMask = 0, boxMask = 0;
    for (int k = 0; k < 9; ++k) {
      const uint16_t rb = (uint16_t)(1 << grid[u * 9 + k]);
      const uint16_t cb = (uint16_t)(1 << grid[k * 9 + u]);
      const int br = (u / 3) * 3 + k / 3, bc = (u % 3) * 3 + k % 3;
      const uint16_t bb = (uint16_t)(1 << grid[br * 9 + bc]);
      if (rowMask & rb) return false;
      if (colMask & cb) return false;
      if (boxMask & bb) return false;
      rowMask |= rb;
      colMask |= cb;
      boxMask |= bb;
    }
  }
  return true;
}

void SudokuActivity::onEnter() {
  Activity::onEnter();
  randomSeed(micros());
  generate();
  requestUpdate(true);
}

void SudokuActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) { finish(); return; }
  if (won) {
    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) { generate(); requestUpdate(); }
    return;
  }
  if (mappedInput.wasPressed(MappedInputManager::Button::Left)) { curC = (curC + 8) % 9; requestUpdate(); }
  else if (mappedInput.wasPressed(MappedInputManager::Button::Right)) { curC = (curC + 1) % 9; requestUpdate(); }
  else if (mappedInput.wasPressed(MappedInputManager::Button::Up)) { curR = (curR + 8) % 9; requestUpdate(); }
  else if (mappedInput.wasPressed(MappedInputManager::Button::Down)) { curR = (curR + 1) % 9; requestUpdate(); }
  else if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    const int pos = curR * 9 + curC;
    if (!fixed[pos]) {
      grid[pos] = (uint8_t)((grid[pos] + 1) % 10);  // blank -> 1..9 -> blank
      if (solved()) won = true;
      requestUpdate();
    }
  }
}

void SudokuActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();

  renderer.clearScreen();
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.homeTopPadding}, "Sudoku");

  const int headerBottom = metrics.headerHeight + metrics.homeTopPadding;
  const int availH = pageHeight - headerBottom - metrics.buttonHintsHeight - 40;
  int cell = (pageWidth * 92 / 100) / 9;
  if (cell > availH / 9) cell = availH / 9;
  const int boardSize = cell * 9;
  const int ox = (pageWidth - boardSize) / 2, oy = headerBottom + 12;

  for (int i = 0; i <= 9; ++i) {
    const int w = (i % 3 == 0) ? 3 : 1;  // thick 3x3 separators
    renderer.drawLine(ox + i * cell, oy, ox + i * cell, oy + boardSize, w, true);
    renderer.drawLine(ox, oy + i * cell, ox + boardSize, oy + i * cell, w, true);
  }

  char buf[2];
  for (int r = 0; r < 9; ++r)
    for (int c = 0; c < 9; ++c) {
      const uint8_t v = grid[r * 9 + c];
      if (v != 0) {
        snprintf(buf, sizeof(buf), "%d", v);
        const auto style = fixed[r * 9 + c] ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR;
        const int tw = renderer.getTextWidth(NOTOSANS_18_FONT_ID, buf, style);
        renderer.drawText(NOTOSANS_18_FONT_ID, ox + c * cell + (cell - tw) / 2, oy + r * cell + cell * 72 / 100, buf,
                          true, style);
      }
    }

  if (!won) renderer.drawRect(ox + curC * cell + 1, oy + curR * cell + 1, cell - 2, cell - 2, 2, true);

  const char* status = won ? "Solved!" : "Fill the grid 1-9";
  renderer.drawCenteredText(UI_12_FONT_ID, oy + boardSize + 24, status, true, EpdFontFamily::BOLD);

  const auto labels = won ? mappedInput.mapLabels(tr(STR_BACK), "New game", "", "")
                          : mappedInput.mapLabels(tr(STR_BACK), "Digit", tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer(HalDisplay::RefreshMode::FAST_REFRESH);
}
