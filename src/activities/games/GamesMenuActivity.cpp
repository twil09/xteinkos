#include "GamesMenuActivity.h"

#include <I18n.h>

#include <memory>

#include "HalDisplay.h"
#include "activities/games/FifteenPuzzleActivity.h"
#include "activities/games/Game2048Activity.h"
#include "activities/games/HangmanActivity.h"
#include "activities/games/LightsOutActivity.h"
#include "activities/games/MemoryMatchActivity.h"
#include "activities/games/SnakeActivity.h"
#include "activities/games/TicTacToeActivity.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
// Games are appended here as each is ported. The label lambdas below read this.
const char* const kGames[] = {"Tic-Tac-Toe", "2048",         "Lights Out", "15 Puzzle",
                              "Memory Match", "Snake",       "Hangman"};
constexpr int kGameCount = sizeof(kGames) / sizeof(kGames[0]);
}  // namespace

int GamesMenuActivity::gameCount() const { return kGameCount; }

void GamesMenuActivity::onEnter() {
  Activity::onEnter();
  requestUpdate(true);
}

void GamesMenuActivity::launch(int index) {
  switch (index) {
    case 0:
      startActivityForResult(std::make_unique<TicTacToeActivity>(renderer, mappedInput),
                             [](const ActivityResult&) {});
      break;
    case 1:
      startActivityForResult(std::make_unique<Game2048Activity>(renderer, mappedInput),
                             [](const ActivityResult&) {});
      break;
    case 2:
      startActivityForResult(std::make_unique<LightsOutActivity>(renderer, mappedInput),
                             [](const ActivityResult&) {});
      break;
    case 3:
      startActivityForResult(std::make_unique<FifteenPuzzleActivity>(renderer, mappedInput),
                             [](const ActivityResult&) {});
      break;
    case 4:
      startActivityForResult(std::make_unique<MemoryMatchActivity>(renderer, mappedInput),
                             [](const ActivityResult&) {});
      break;
    case 5:
      startActivityForResult(std::make_unique<SnakeActivity>(renderer, mappedInput),
                             [](const ActivityResult&) {});
      break;
    case 6:
      startActivityForResult(std::make_unique<HangmanActivity>(renderer, mappedInput),
                             [](const ActivityResult&) {});
      break;
    default:
      break;
  }
}

void GamesMenuActivity::loop() {
  if (mappedInput.wasPressed(MappedInputManager::Button::Up)) {
    selectorIndex = (selectorIndex + gameCount() - 1) % gameCount();
    requestUpdate();
    return;
  }
  if (mappedInput.wasPressed(MappedInputManager::Button::Down)) {
    selectorIndex = (selectorIndex + 1) % gameCount();
    requestUpdate();
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    launch(selectorIndex);
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    finish();
  }
}

void GamesMenuActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();

  renderer.clearScreen();
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.homeTopPadding}, tr(STR_GAMES));

  const int top = metrics.headerHeight + metrics.homeTopPadding + metrics.homeMenuTopOffset;
  GUI.drawButtonMenu(
      renderer, Rect{0, top, pageWidth, pageHeight - top - metrics.buttonHintsHeight}, gameCount(), selectorIndex,
      [](int i) { return std::string(kGames[i]); }, [](int) { return Book; });

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer();
}
