#include "FifteenPuzzleActivity.h"

#include <Arduino.h>
#include <I18n.h>

#include <cstdio>

#include "HalDisplay.h"
#include "components/UITheme.h"
#include "fontIds.h"

// Move the blank in a direction (the tile there slides in). Returns true if moved.
bool FifteenPuzzleActivity::moveBlank(int dir) {
  const int r = blank / 4, c = blank % 4;
  int nr = r, nc = c;
  if (dir == 0) nc = c - 1;
  else if (dir == 1) nc = c + 1;
  else if (dir == 2) nr = r - 1;
  else nr = r + 1;
  if (nr < 0 || nr > 3 || nc < 0 || nc > 3) return false;
  const int np = nr * 4 + nc;
  tiles[blank] = tiles[np];
  tiles[np] = 0;
  blank = np;
  return true;
}

void FifteenPuzzleActivity::reset() {
  for (int i = 0; i < 15; ++i) tiles[i] = i + 1;
  tiles[15] = 0;
  blank = 15;
  // Shuffle by random valid blank moves (keeps the puzzle solvable).
  for (int i = 0; i < 200; ++i) moveBlank(random(4));
  moves = 0;
  solved = false;
  if (isSolved()) moveBlank(0), moveBlank(1);  // avoid starting solved
}

void FifteenPuzzleActivity::onEnter() {
  Activity::onEnter();
  reset();
  requestUpdate(true);
}

bool FifteenPuzzleActivity::isSolved() const {
  for (int i = 0; i < 15; ++i)
    if (tiles[i] != i + 1) return false;
  return tiles[15] == 0;
}

void FifteenPuzzleActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) { finish(); return; }
  if (solved) {
    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) { reset(); requestUpdate(); }
    return;
  }
  int dir = -1;
  if (mappedInput.wasPressed(MappedInputManager::Button::Left)) dir = 0;
  else if (mappedInput.wasPressed(MappedInputManager::Button::Right)) dir = 1;
  else if (mappedInput.wasPressed(MappedInputManager::Button::Up)) dir = 2;
  else if (mappedInput.wasPressed(MappedInputManager::Button::Down)) dir = 3;
  if (dir < 0) return;
  if (moveBlank(dir)) {
    ++moves;
    if (isSolved()) solved = true;
    requestUpdate();
  }
}

void FifteenPuzzleActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();

  renderer.clearScreen();
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.homeTopPadding}, "15 Puzzle");

  const int headerBottom = metrics.headerHeight + metrics.homeTopPadding;
  const int avail = pageHeight - headerBottom - metrics.buttonHintsHeight - 60;
  int boardSize = (pageWidth < avail ? pageWidth : avail) * 90 / 100;
  const int cell = boardSize / 4;
  boardSize = cell * 4;
  const int bx = (pageWidth - boardSize) / 2;
  const int by = headerBottom + 20;

  char buf[8];
  for (int i = 0; i < 16; ++i) {
    if (tiles[i] == 0) continue;
    const int x = bx + (i % 4) * cell, y = by + (i / 4) * cell;
    renderer.drawRoundedRect(x + 3, y + 3, cell - 6, cell - 6, 2, 6, true);
    snprintf(buf, sizeof(buf), "%d", tiles[i]);
    const int tw = renderer.getTextWidth(NOTOSANS_16_FONT_ID, buf);
    renderer.drawText(NOTOSANS_16_FONT_ID, x + (cell - tw) / 2, y + cell * 62 / 100, buf, true,
                      EpdFontFamily::BOLD);
  }

  const char* status = solved ? "Solved!" : "Order the tiles 1-15";
  renderer.drawCenteredText(UI_12_FONT_ID, by + boardSize + 30, status, true, EpdFontFamily::BOLD);

  const auto labels = solved ? mappedInput.mapLabels(tr(STR_BACK), "New game", "", "")
                             : mappedInput.mapLabels(tr(STR_BACK), "", tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer(HalDisplay::RefreshMode::FAST_REFRESH);
}
