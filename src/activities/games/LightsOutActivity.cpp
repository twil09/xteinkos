#include "LightsOutActivity.h"

#include <Arduino.h>
#include <I18n.h>

#include "HalDisplay.h"
#include "components/UITheme.h"
#include "fontIds.h"

void LightsOutActivity::press(int r, int c) {
  if (r < 0 || r >= N || c < 0 || c >= N) return;
  grid[r][c] = !grid[r][c];
  if (r > 0) grid[r - 1][c] = !grid[r - 1][c];
  if (r < N - 1) grid[r + 1][c] = !grid[r + 1][c];
  if (c > 0) grid[r][c - 1] = !grid[r][c - 1];
  if (c < N - 1) grid[r][c + 1] = !grid[r][c + 1];
}

void LightsOutActivity::reset() {
  for (int r = 0; r < N; ++r)
    for (int c = 0; c < N; ++c) grid[r][c] = false;
  // Apply random presses so the start state is always solvable.
  for (int i = 0; i < 8; ++i) press(random(N), random(N));
  if (allOff()) press(random(N), random(N));  // never start solved
  curR = curC = 2;
  moves = 0;
  solved = false;
}

void LightsOutActivity::onEnter() {
  Activity::onEnter();
  reset();
  requestUpdate(true);
}

bool LightsOutActivity::allOff() const {
  for (int r = 0; r < N; ++r)
    for (int c = 0; c < N; ++c)
      if (grid[r][c]) return false;
  return true;
}

void LightsOutActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) { finish(); return; }
  if (solved) {
    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) { reset(); requestUpdate(); }
    return;
  }
  if (mappedInput.wasPressed(MappedInputManager::Button::Left)) { curC = (curC + N - 1) % N; requestUpdate(); }
  else if (mappedInput.wasPressed(MappedInputManager::Button::Right)) { curC = (curC + 1) % N; requestUpdate(); }
  else if (mappedInput.wasPressed(MappedInputManager::Button::Up)) { curR = (curR + N - 1) % N; requestUpdate(); }
  else if (mappedInput.wasPressed(MappedInputManager::Button::Down)) { curR = (curR + 1) % N; requestUpdate(); }
  else if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    press(curR, curC);
    ++moves;
    if (allOff()) solved = true;
    requestUpdate();
  }
}

void LightsOutActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();

  renderer.clearScreen();
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.homeTopPadding}, "Lights Out");

  const int headerBottom = metrics.headerHeight + metrics.homeTopPadding;
  const int avail = pageHeight - headerBottom - metrics.buttonHintsHeight - 60;
  int boardSize = (pageWidth < avail ? pageWidth : avail) * 90 / 100;
  const int cell = boardSize / N;
  boardSize = cell * N;
  const int bx = (pageWidth - boardSize) / 2;
  const int by = headerBottom + 20;

  for (int r = 0; r < N; ++r)
    for (int c = 0; c < N; ++c) {
      const int x = bx + c * cell, y = by + r * cell;
      if (grid[r][c])
        renderer.fillRoundedRect(x + 3, y + 3, cell - 6, cell - 6, 6, Color::Black);
      else
        renderer.drawRoundedRect(x + 3, y + 3, cell - 6, cell - 6, 2, 6, true);
      if (!solved && r == curR && c == curC)
        renderer.drawRect(x + 1, y + 1, cell - 2, cell - 2, 2, true);
    }

  const char* status = solved ? "Solved!" : "Turn them all off";
  renderer.drawCenteredText(UI_12_FONT_ID, by + boardSize + 30, status, true, EpdFontFamily::BOLD);

  const auto labels = solved ? mappedInput.mapLabels(tr(STR_BACK), "New game", "", "")
                             : mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer(HalDisplay::RefreshMode::FAST_REFRESH);
}
