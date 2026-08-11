#include "SnakeActivity.h"

#include <Arduino.h>
#include <I18n.h>

#include <cstdio>

#include "HalDisplay.h"
#include "components/UITheme.h"
#include "fontIds.h"

static constexpr unsigned long STEP_MS = 260;

bool SnakeActivity::onSnake(int x, int y, int upto) const {
  for (int i = 0; i < upto; ++i)
    if (bx[i] == x && by[i] == y) return true;
  return false;
}

void SnakeActivity::spawnFood() {
  for (int tries = 0; tries < 400; ++tries) {
    int x = random(COLS), y = random(ROWS);
    if (!onSnake(x, y, len)) { foodX = x; foodY = y; return; }
  }
}

void SnakeActivity::reset() {
  len = 3;
  const int cx = COLS / 2, cy = ROWS / 2;
  for (int i = 0; i < len; ++i) { bx[i] = cx - i; by[i] = cy; }
  dir = nextDir = 3;
  score = 0;
  over = false;
  spawnFood();
  lastStep = millis();
}

void SnakeActivity::onEnter() {
  Activity::onEnter();
  reset();
  requestUpdate(true);
}

void SnakeActivity::step() {
  dir = nextDir;
  int nx = bx[0], ny = by[0];
  if (dir == 0) --ny; else if (dir == 1) ++ny; else if (dir == 2) --nx; else ++nx;
  if (nx < 0 || nx >= COLS || ny < 0 || ny >= ROWS) { over = true; return; }
  const bool grow = (nx == foodX && ny == foodY);
  if (onSnake(nx, ny, grow ? len : len - 1)) { over = true; return; }
  if (grow && len < MAX) ++len;
  for (int i = len - 1; i > 0; --i) { bx[i] = bx[i - 1]; by[i] = by[i - 1]; }
  bx[0] = nx; by[0] = ny;
  if (grow) { ++score; spawnFood(); }
}

void SnakeActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) { finish(); return; }

  // Buffer a direction change (no 180-degree reversal).
  if (mappedInput.wasPressed(MappedInputManager::Button::Up) && dir != 1) nextDir = 0;
  else if (mappedInput.wasPressed(MappedInputManager::Button::Down) && dir != 0) nextDir = 1;
  else if (mappedInput.wasPressed(MappedInputManager::Button::Left) && dir != 3) nextDir = 2;
  else if (mappedInput.wasPressed(MappedInputManager::Button::Right) && dir != 2) nextDir = 3;

  if (over) {
    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) { reset(); requestUpdate(); }
    return;
  }
  if (millis() - lastStep >= STEP_MS) {
    lastStep = millis();
    step();
    requestUpdate();
  }
}

void SnakeActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();

  renderer.clearScreen();
  char title[24];
  snprintf(title, sizeof(title), "Snake  -  %d", score);
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.homeTopPadding}, title);

  const int headerBottom = metrics.headerHeight + metrics.homeTopPadding;
  const int availH = pageHeight - headerBottom - metrics.buttonHintsHeight - 40;
  int cell = (pageWidth * 94 / 100) / COLS;
  if (cell > availH / ROWS) cell = availH / ROWS;
  const int boardW = cell * COLS, boardH = cell * ROWS;
  const int ox = (pageWidth - boardW) / 2;
  const int oy = headerBottom + 10;

  renderer.drawRect(ox - 2, oy - 2, boardW + 4, boardH + 4, 2, true);
  // food (hollow square)
  renderer.drawRoundedRect(ox + foodX * cell + 1, oy + foodY * cell + 1, cell - 2, cell - 2, 2, 3, true);
  // snake (filled)
  for (int i = 0; i < len; ++i)
    renderer.fillRoundedRect(ox + bx[i] * cell + 1, oy + by[i] * cell + 1, cell - 2, cell - 2, 2, Color::Black);

  if (over)
    renderer.drawCenteredText(UI_12_FONT_ID, oy + boardH + 26, "Game over", true, EpdFontFamily::BOLD);

  const auto labels = over ? mappedInput.mapLabels(tr(STR_BACK), "Retry", "", "")
                           : mappedInput.mapLabels(tr(STR_BACK), "", tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer(HalDisplay::RefreshMode::FAST_REFRESH);
}
