#include "FilesApp.h"

#include "AppManager.h"
#include "LibraryApp.h"
#include "theme.h"

bool FilesApp::onButton(Btn b) {
  switch (b) {
    case Btn::Up:
    case Btn::Left:    sel_ = (sel_ + 1) % 2; return true;
    case Btn::Down:
    case Btn::Right:   sel_ = (sel_ + 1) % 2; return true;
    case Btn::Confirm:
      nav->push(new LibraryApp(sel_ == 0 ? LibraryApp::Cat::Books
                                         : LibraryApp::Cat::Images));
      return true;
    default: return false;  // Back -> home
  }
}

void FilesApp::render(DuetDisplay& d) {
  auto& g = d.gfx();
  duet::headerBar(g, "Files", "");
  const int rowH = 90;
  int y = UI_HEADER_H + 20;
  duet::listRow(g, 0, y, SCREEN_W, rowH, "Books   (.txt .epub .pdf ...)", sel_ == 0);
  duet::listRow(g, 0, y + rowH + 10, SCREEN_W, rowH, "Images  (.jpg .png ...)", sel_ == 1);
  duet::buttonBar(g, "Back", "Open", "Up", "Down");
}
