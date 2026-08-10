#include "GamesApp.h"

#include "AppManager.h"
#include "theme.h"
#include "Blackjack.h"
#include "ConnectFour.h"
#include "FifteenPuzzle.h"
#include "Game2048.h"
#include "Hangman.h"
#include "LightsOut.h"
#include "MemoryMatch.h"
#include "Minesweeper.h"
#include "Reversi.h"
#include "Snake.h"
#include "Sudoku.h"
#include "TicTacToe.h"

namespace {
const char* kGames[] = {
    "2048",        "Tic-Tac-Toe", "Sudoku",       "Minesweeper",
    "15 Puzzle",   "Lights Out",  "Snake",        "Memory Match",
    "Connect Four", "Reversi",    "Hangman",      "Blackjack",
};
const int kCount = sizeof(kGames) / sizeof(kGames[0]);
}  // namespace

void GamesApp::launch(int i) {
  switch (i) {
    case 0:  nav->push(new Game2048()); break;
    case 1:  nav->push(new TicTacToe()); break;
    case 2:  nav->push(new Sudoku()); break;
    case 3:  nav->push(new Minesweeper()); break;
    case 4:  nav->push(new FifteenPuzzle()); break;
    case 5:  nav->push(new LightsOut()); break;
    case 6:  nav->push(new Snake()); break;
    case 7:  nav->push(new MemoryMatch()); break;
    case 8:  nav->push(new ConnectFour()); break;
    case 9:  nav->push(new Reversi()); break;
    case 10: nav->push(new Hangman()); break;
    case 11: nav->push(new Blackjack()); break;
    default: break;
  }
}

bool GamesApp::onButton(Btn b) {
  switch (b) {
    case Btn::Up:
    case Btn::Left:    sel_ = (sel_ - 1 + kCount) % kCount; return true;
    case Btn::Down:
    case Btn::Right:   sel_ = (sel_ + 1) % kCount;          return true;
    case Btn::Confirm: launch(sel_);                        return true;
    default:           return false;  // Back -> home
  }
}

void GamesApp::render(DuetDisplay& d) {
  auto& g = d.gfx();
  duet::headerBar(g, "Games", String(kCount) + " apps");

  const int rowH = 58;
  const int rows = (SCREEN_H - UI_HEADER_H - UI_FOOTER_H) / rowH;
  if (sel_ < scrollTop_) scrollTop_ = sel_;
  if (sel_ >= scrollTop_ + rows) scrollTop_ = sel_ - rows + 1;
  if (scrollTop_ < 0) scrollTop_ = 0;

  int y = UI_HEADER_H;
  for (int i = 0; i < rows; ++i) {
    int idx = scrollTop_ + i;
    if (idx >= kCount) break;
    duet::listRow(g, 0, y, SCREEN_W, rowH, kGames[idx], idx == sel_);
    y += rowH;
  }
  duet::buttonBar(g, "Back", "Open", "Up", "Down");
}
