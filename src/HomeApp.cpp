#include "HomeApp.h"

#include "AppManager.h"
#include "theme.h"
// games
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
// apps
#include "LibraryApp.h"
#include "QrApp.h"
#include "WiFiApp.h"

namespace {
const char* kEntries[] = {
    "2048",         "Tic-Tac-Toe",  "Sudoku",      "Minesweeper",
    "15 Puzzle",    "Lights Out",   "Snake",       "Memory Match",
    "Connect Four (2P)", "Reversi",  "Hangman",     "Blackjack",
    "Library  (read books)", "Wi-Fi Setup", "QR Codes",
};
const int kCount = sizeof(kEntries) / sizeof(kEntries[0]);
}  // namespace

void HomeApp::launch(int i) {
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
    case 12: nav->push(new LibraryApp()); break;
    case 13: nav->push(new WiFiApp()); break;
    case 14: nav->push(new QrApp()); break;
    default: break;
  }
}

bool HomeApp::onButton(Btn b) {
  switch (b) {
    case Btn::Up:      sel_ = (sel_ - 1 + kCount) % kCount; return true;
    case Btn::Down:    sel_ = (sel_ + 1) % kCount;         return true;
    case Btn::Confirm: launch(sel_);                       return true;
    default:           return false;
  }
}

void HomeApp::render(DuetDisplay& d) {
  auto& g = d.gfx();
  duet::header(g, String(FW_NAME) + "   v" FW_VERSION);

  const int rowH = 62;
  const int rows = (SCREEN_H - UI_HEADER_H - UI_FOOTER_H) / rowH;
  if (sel_ < scrollTop_) scrollTop_ = sel_;
  if (sel_ >= scrollTop_ + rows) scrollTop_ = sel_ - rows + 1;
  if (scrollTop_ < 0) scrollTop_ = 0;

  int y = UI_HEADER_H;
  for (int i = 0; i < rows; ++i) {
    int idx = scrollTop_ + i;
    if (idx >= kCount) break;
    duet::listRow(g, 0, y, SCREEN_W, rowH, kEntries[idx], idx == sel_);
    y += rowH;
  }
  String pos = String(sel_ + 1) + "/" + String(kCount);
  duet::footerHint(g, "Up/Down: select    Confirm: open        " + pos);
}
