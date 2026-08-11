#include "FilesApp.h"

#include "AppManager.h"
#include "LibraryApp.h"
#include "OpdsApp.h"
#include "theme.h"

bool FilesApp::onButton(Btn b) {
  const int rows = 3;
  switch (b) {
    case Btn::Up:
    case Btn::Left:    sel_ = (sel_ + rows - 1) % rows; return true;
    case Btn::Down:
    case Btn::Right:   sel_ = (sel_ + 1) % rows; return true;
    case Btn::Confirm:
      if (sel_ == 0) nav->push(new LibraryApp(LibraryApp::Cat::Books));
      else if (sel_ == 1) nav->push(new LibraryApp(LibraryApp::Cat::Images));
      else nav->push(new OpdsApp());
      return true;
    default: return false;  // Back -> home
  }
}

void FilesApp::render(DuetDisplay& d) {
  auto& g = d.gfx();
  duet::headerBar(g, "Books", "");
  const int rowH = 82;
  int y = UI_HEADER_H + 18;
  duet::listRow(g, 0, y, SCREEN_W, rowH, "Books   (.txt .epub .html ...)", sel_ == 0);
  duet::listRow(g, 0, y + rowH + 8, SCREEN_W, rowH, "Images  (.jpg .png ...)", sel_ == 1);
  duet::listRow(g, 0, y + 2 * (rowH + 8), SCREEN_W, rowH,
                "Download books  (OPDS, Wi-Fi)", sel_ == 2);
  duet::buttonBar(g, "Back", "Open", "Up", "Down");
}
