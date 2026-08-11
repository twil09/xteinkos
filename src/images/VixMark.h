#pragma once
#include <GfxRenderer.h>

// The Vix "V" mark, drawn with vector strokes rather than a bitmap so it stays
// upright in any panel orientation (drawImage does not rotate its bits, but the
// line primitives go through the orientation-aware pixel path). Centered in a
// `size`x`size` box at (x, y).
inline void drawVixMark(const GfxRenderer& renderer, int x, int y, int size) {
  const int apexX = x + size / 2;
  const int apexY = y + (size * 86) / 100;
  const int topY = y + (size * 12) / 100;
  const int lw = size * 13 / 100;  // ~16px stroke at size 120
  renderer.drawLine(x + (size * 16) / 100, topY, apexX, apexY, lw, true);
  renderer.drawLine(x + (size * 84) / 100, topY, apexX, apexY, lw, true);
}
