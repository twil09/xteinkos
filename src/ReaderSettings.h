// ReaderSettings — global reading preferences (font family/size, line spacing,
// margins), persisted to LittleFS /reader.json. Shared by every open book.
#pragma once
#include <Adafruit_GFX.h>

namespace ReaderSettings {

void load();
void save();

// Active body font + derived metrics for the current settings.
const GFXfont* font();
int lineHeight();   // vertical pixels per line
int margin();       // left/right margin in pixels

// Adjustable axes (each wraps/clamps and persists on change).
int  family();      // 0 = Sans, 1 = Serif
void setFamily(int);
const char* familyName();

int  sizeIdx();     // 0..3 -> 9/12/18/24 pt
void setSizeIdx(int);
const char* sizeName();

int  spacingIdx();  // 0 tight, 1 normal, 2 loose
void setSpacingIdx(int);
const char* spacingName();

int  marginIdx();   // 0 narrow, 1 normal, 2 wide
void setMarginIdx(int);
const char* marginName();

}  // namespace ReaderSettings
