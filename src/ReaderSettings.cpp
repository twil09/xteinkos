#include "ReaderSettings.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSerif12pt7b.h>
#include <Fonts/FreeSerif18pt7b.h>
#include <Fonts/FreeSerif24pt7b.h>
#include <Fonts/FreeSerif9pt7b.h>

#define READER_CFG "/reader.json"

namespace {
int g_family = 0;    // 0 sans, 1 serif
int g_size = 0;      // 0..3
int g_spacing = 1;   // 0 tight, 1 normal, 2 loose
int g_margin = 1;    // 0 narrow, 1 normal, 2 wide
bool g_loaded = false;

const GFXfont* kSans[4] = {&FreeSans9pt7b, &FreeSans12pt7b, &FreeSans18pt7b,
                           &FreeSans24pt7b};
const GFXfont* kSerif[4] = {&FreeSerif9pt7b, &FreeSerif12pt7b, &FreeSerif18pt7b,
                            &FreeSerif24pt7b};
// Base line height per size (px); scaled by spacing.
const int kBase[4] = {24, 30, 42, 56};
const float kSpace[3] = {0.86f, 1.0f, 1.18f};
const int kMargins[3] = {14, 22, 36};

void clampAll() {
  g_family = (g_family % 2 + 2) % 2;
  if (g_size < 0) g_size = 0; if (g_size > 3) g_size = 3;
  if (g_spacing < 0) g_spacing = 0; if (g_spacing > 2) g_spacing = 2;
  if (g_margin < 0) g_margin = 0; if (g_margin > 2) g_margin = 2;
}
}  // namespace

namespace ReaderSettings {

void load() {
  if (g_loaded) return;
  g_loaded = true;
  File f = LittleFS.open(READER_CFG, "r");
  if (!f) return;
  JsonDocument doc;
  if (deserializeJson(doc, f) == DeserializationError::Ok) {
    g_family = doc["family"] | 0;
    g_size = doc["size"] | 0;
    g_spacing = doc["spacing"] | 1;
    g_margin = doc["margin"] | 1;
    clampAll();
  }
  f.close();
}

void save() {
  JsonDocument doc;
  doc["family"] = g_family;
  doc["size"] = g_size;
  doc["spacing"] = g_spacing;
  doc["margin"] = g_margin;
  File f = LittleFS.open(READER_CFG, "w");
  if (f) { serializeJson(doc, f); f.close(); }
}

const GFXfont* font() { load(); return (g_family ? kSerif : kSans)[g_size]; }
int lineHeight() { load(); return (int)(kBase[g_size] * kSpace[g_spacing]); }
int margin() { load(); return kMargins[g_margin]; }

int family() { load(); return g_family; }
void setFamily(int v) { load(); g_family = v; clampAll(); save(); }
const char* familyName() { load(); return g_family ? "Serif" : "Sans"; }

int sizeIdx() { load(); return g_size; }
void setSizeIdx(int v) { load(); g_size = v; clampAll(); save(); }
const char* sizeName() {
  load();
  static const char* n[4] = {"Small", "Medium", "Large", "X-Large"};
  return n[g_size];
}

int spacingIdx() { load(); return g_spacing; }
void setSpacingIdx(int v) { load(); g_spacing = v; clampAll(); save(); }
const char* spacingName() {
  load();
  static const char* n[3] = {"Tight", "Normal", "Loose"};
  return n[g_spacing];
}

int marginIdx() { load(); return g_margin; }
void setMarginIdx(int v) { load(); g_margin = v; clampAll(); save(); }
const char* marginName() {
  load();
  static const char* n[3] = {"Narrow", "Normal", "Wide"};
  return n[g_margin];
}

}  // namespace ReaderSettings
