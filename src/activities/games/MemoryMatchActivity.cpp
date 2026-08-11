#include "MemoryMatchActivity.h"

#include <Arduino.h>
#include <I18n.h>

#include <cstdio>

#include "HalDisplay.h"
#include "components/UITheme.h"
#include "fontIds.h"

void MemoryMatchActivity::reset() {
  for (int i = 0; i < COUNT; ++i) {
    value[i] = i / 2 + 1;  // 1,1,2,2,...,8,8
    matched[i] = false;
  }
  for (int i = COUNT - 1; i > 0; --i) {  // Fisher-Yates shuffle
    int j = random(i + 1);
    int t = value[i]; value[i] = value[j]; value[j] = t;
  }
  cursor = 0;
  firstPick = -1;
  pendingA = pendingB = -1;
  pairsFound = 0;
  won = false;
}

void MemoryMatchActivity::onEnter() {
  Activity::onEnter();
  reset();
  requestUpdate(true);
}

bool MemoryMatchActivity::shown(int i) const {
  return i == firstPick || i == pendingA || i == pendingB;
}

void MemoryMatchActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) { finish(); return; }
  if (won) {
    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) { reset(); requestUpdate(); }
    return;
  }
  // A shown mismatch is dismissed by the next input.
  if (pendingA >= 0) {
    if (mappedInput.wasAnyReleased() || mappedInput.wasAnyPressed()) {
      pendingA = pendingB = -1;
      requestUpdate();
    }
    return;
  }

  if (mappedInput.wasPressed(MappedInputManager::Button::Left)) {
    cursor = (cursor % COLS == 0) ? cursor + COLS - 1 : cursor - 1; requestUpdate();
  } else if (mappedInput.wasPressed(MappedInputManager::Button::Right)) {
    cursor = (cursor % COLS == COLS - 1) ? cursor - COLS + 1 : cursor + 1; requestUpdate();
  } else if (mappedInput.wasPressed(MappedInputManager::Button::Up)) {
    cursor = (cursor + COUNT - COLS) % COUNT; requestUpdate();
  } else if (mappedInput.wasPressed(MappedInputManager::Button::Down)) {
    cursor = (cursor + COLS) % COUNT; requestUpdate();
  } else if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    if (matched[cursor] || cursor == firstPick) return;
    if (firstPick < 0) {
      firstPick = cursor;
    } else {
      if (value[cursor] == value[firstPick]) {
        matched[cursor] = matched[firstPick] = true;
        firstPick = -1;
        if (++pairsFound == COUNT / 2) won = true;
      } else {
        pendingA = firstPick;
        pendingB = cursor;
        firstPick = -1;
      }
    }
    requestUpdate();
  }
}

void MemoryMatchActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();

  renderer.clearScreen();
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.homeTopPadding}, "Memory Match");

  const int headerBottom = metrics.headerHeight + metrics.homeTopPadding;
  const int avail = pageHeight - headerBottom - metrics.buttonHintsHeight - 60;
  int boardW = (pageWidth < avail ? pageWidth : avail) * 92 / 100;
  const int cell = boardW / COLS;
  boardW = cell * COLS;
  const int bx = (pageWidth - boardW) / 2;
  const int by = headerBottom + 20;

  char buf[8];
  for (int i = 0; i < COUNT; ++i) {
    const int x = bx + (i % COLS) * cell, y = by + (i / COLS) * cell;
    if (matched[i]) continue;  // cleared
    renderer.drawRoundedRect(x + 3, y + 3, cell - 6, cell - 6, 2, 6, true);
    if (shown(i)) {
      snprintf(buf, sizeof(buf), "%d", value[i]);
      const int tw = renderer.getTextWidth(NOTOSANS_16_FONT_ID, buf);
      renderer.drawText(NOTOSANS_16_FONT_ID, x + (cell - tw) / 2, y + cell * 62 / 100, buf, true,
                        EpdFontFamily::BOLD);
    } else {
      const int tw = renderer.getTextWidth(NOTOSANS_16_FONT_ID, "?");
      renderer.drawText(NOTOSANS_16_FONT_ID, x + (cell - tw) / 2, y + cell * 62 / 100, "?", true);
    }
    if (!won && i == cursor) renderer.drawRect(x + 1, y + 1, cell - 2, cell - 2, 2, true);
  }

  if (won)
    renderer.drawCenteredText(UI_12_FONT_ID, by + cell * ROWS + 30, "You win!", true, EpdFontFamily::BOLD);
  else {
    snprintf(buf, sizeof(buf), "%d/%d", pairsFound, COUNT / 2);
    renderer.drawCenteredText(UI_12_FONT_ID, by + cell * ROWS + 30, buf, true, EpdFontFamily::BOLD);
  }

  const auto labels = won ? mappedInput.mapLabels(tr(STR_BACK), "New game", "", "")
                          : mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer(HalDisplay::RefreshMode::FAST_REFRESH);
}
