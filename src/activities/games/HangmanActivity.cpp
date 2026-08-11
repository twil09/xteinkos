#include "HangmanActivity.h"

#include <Arduino.h>
#include <I18n.h>

#include <cstdio>
#include <cstring>

#include "HalDisplay.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
const char* const WORDS[] = {"READER",  "BATTERY", "SCREEN",  "GARDEN",  "PLANET",  "LIBRARY",
                             "JOURNEY", "PUZZLE",  "VOYAGE",  "LANTERN", "HARVEST", "ORCHARD",
                             "THUNDER", "WHISPER", "CRYSTAL", "DIAMOND", "FALCON",  "HORIZON",
                             "MEADOW",  "COMPASS", "TWILIGHT", "MARBLE"};
constexpr int WORD_COUNT = sizeof(WORDS) / sizeof(WORDS[0]);
}  // namespace

void HangmanActivity::reset() {
  strncpy(word, WORDS[random(WORD_COUNT)], sizeof(word) - 1);
  word[sizeof(word) - 1] = 0;
  for (int i = 0; i < 26; ++i) guessed[i] = false;
  wrong = 0;
  cursor = 0;
  state = 0;
}

void HangmanActivity::onEnter() {
  Activity::onEnter();
  reset();
  requestUpdate(true);
}

bool HangmanActivity::isWin() const {
  for (const char* p = word; *p; ++p)
    if (!guessed[*p - 'A']) return false;
  return true;
}

void HangmanActivity::guessLetter(int l) {
  if (l < 0 || l >= 26 || guessed[l]) return;
  guessed[l] = true;
  if (!strchr(word, 'A' + l)) {
    if (++wrong >= MAX_WRONG) state = 2;
  }
  if (state == 0 && isWin()) state = 1;
}

void HangmanActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) { finish(); return; }
  if (state != 0) {
    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) { reset(); requestUpdate(); }
    return;
  }
  if (mappedInput.wasPressed(MappedInputManager::Button::Left)) { if (cursor > 0) --cursor; requestUpdate(); }
  else if (mappedInput.wasPressed(MappedInputManager::Button::Right)) { if (cursor < 25) ++cursor; requestUpdate(); }
  else if (mappedInput.wasPressed(MappedInputManager::Button::Up)) { if (cursor >= COLS) cursor -= COLS; requestUpdate(); }
  else if (mappedInput.wasPressed(MappedInputManager::Button::Down)) { if (cursor + COLS <= 25) cursor += COLS; requestUpdate(); }
  else if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) { guessLetter(cursor); requestUpdate(); }
}

void HangmanActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();

  renderer.clearScreen();
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.homeTopPadding}, "Hangman");
  const int headerBottom = metrics.headerHeight + metrics.homeTopPadding;

  // Masked word.
  char masked[40];
  int m = 0;
  for (const char* p = word; *p && m < (int)sizeof(masked) - 2; ++p) {
    masked[m++] = (state == 2 || guessed[*p - 'A']) ? *p : '_';
    masked[m++] = ' ';
  }
  masked[m] = 0;
  renderer.drawCenteredText(NOTOSANS_18_FONT_ID, headerBottom + 40, masked, true, EpdFontFamily::BOLD);

  char miss[24];
  snprintf(miss, sizeof(miss), "Misses: %d / %d", wrong, MAX_WRONG);
  renderer.drawCenteredText(UI_12_FONT_ID, headerBottom + 76, miss, true);

  // Alphabet keyboard (7 columns).
  const int rows = (26 + COLS - 1) / COLS;
  const int kbTop = headerBottom + 100;
  const int cellW = (pageWidth - 24) / COLS;
  const int cellH = cellW;
  const int kbLeft = (pageWidth - cellW * COLS) / 2;
  for (int i = 0; i < 26; ++i) {
    const int r = i / COLS, c = i % COLS;
    const int x = kbLeft + c * cellW, y = kbTop + r * cellH;
    char ch[2] = {(char)('A' + i), 0};
    const bool used = guessed[i];
    if (state == 0 && i == cursor) renderer.fillRoundedRect(x + 2, y + 2, cellW - 4, cellH - 4, 5, Color::Black);
    else renderer.drawRoundedRect(x + 2, y + 2, cellW - 4, cellH - 4, 1, 5, true);
    const bool white = (state == 0 && i == cursor);
    const int tw = renderer.getTextWidth(UI_12_FONT_ID, ch, EpdFontFamily::BOLD);
    // Guessed-but-not-current letters are drawn lighter by outlining only.
    if (!used || white || (state == 0 && i == cursor))
      renderer.drawText(UI_12_FONT_ID, x + (cellW - tw) / 2, y + cellH * 64 / 100, ch, !white, EpdFontFamily::BOLD);
    else
      renderer.drawText(UI_12_FONT_ID, x + (cellW - tw) / 2, y + cellH * 64 / 100, ch, true);
  }

  const int statusY = kbTop + rows * cellH + 26;
  if (state == 1) renderer.drawCenteredText(UI_12_FONT_ID, statusY, "You got it!", true, EpdFontFamily::BOLD);
  else if (state == 2) {
    char lost[40];
    snprintf(lost, sizeof(lost), "Lost - word: %s", word);
    renderer.drawCenteredText(UI_12_FONT_ID, statusY, lost, true, EpdFontFamily::BOLD);
  }

  const auto labels = state != 0 ? mappedInput.mapLabels(tr(STR_BACK), "New word", "", "")
                                 : mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer(HalDisplay::RefreshMode::FAST_REFRESH);
}
