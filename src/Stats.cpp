#include "Stats.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

namespace {
JsonDocument gDoc;
bool gLoaded = false;

void load() {
  if (gLoaded) return;
  File f = LittleFS.open(STATS_PATH, "r");
  if (f) { deserializeJson(gDoc, f); f.close(); }
  gLoaded = true;
}
void store() {
  File w = LittleFS.open(STATS_PATH, "w");
  if (!w) return;
  serializeJson(gDoc, w);
  w.close();
}
}  // namespace

namespace Stats {

void addSession(uint32_t ms, int pages) {
  load();
  gDoc["ms"] = (uint32_t)(gDoc["ms"] | (uint32_t)0) + ms;
  gDoc["pages"] = (uint32_t)(gDoc["pages"] | (uint32_t)0) + (pages > 0 ? pages : 0);
  store();
}

uint32_t totalMinutes() { load(); return (uint32_t)(gDoc["ms"] | (uint32_t)0) / 60000UL; }
uint32_t totalPages()   { load(); return gDoc["pages"] | (uint32_t)0; }

int speedScore() {
  load();
  uint32_t ms = gDoc["ms"] | (uint32_t)0;
  uint32_t pages = gDoc["pages"] | (uint32_t)0;
  if (pages < 3) return 0;
  float secPerPage = (ms / 1000.0f) / (float)pages;
  const float FAST = 12.0f, SLOW = 90.0f;  // 12s/page -> 100, 90s/page -> 1
  float t = (secPerPage - FAST) / (SLOW - FAST);
  int score = (int)(100.0f * (1.0f - t) + 0.5f);
  if (score < 1) score = 1;
  if (score > 100) score = 100;
  return score;
}

void setCompleted(const String& key, bool done) {
  load();
  if (!gDoc["done"].is<JsonObject>()) gDoc["done"].to<JsonObject>();
  gDoc["done"][key] = done;
  store();
}

bool isCompleted(const String& key) {
  load();
  return gDoc["done"][key] | false;
}

}  // namespace Stats
