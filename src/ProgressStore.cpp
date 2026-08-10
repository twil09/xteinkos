#include "ProgressStore.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

#define READER_PROGRESS_PATH "/reader.json"

namespace ProgressStore {

uint32_t load(const String& path) {
  File f = LittleFS.open(READER_PROGRESS_PATH, "r");
  if (!f) return 0;
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return 0;
  return doc[path] | (uint32_t)0;
}

void save(const String& path, uint32_t offset) {
  JsonDocument doc;
  File f = LittleFS.open(READER_PROGRESS_PATH, "r");
  if (f) { deserializeJson(doc, f); f.close(); }
  doc[path] = offset;
  File w = LittleFS.open(READER_PROGRESS_PATH, "w");
  if (!w) return;
  serializeJson(doc, w);
  w.close();
}

}  // namespace ProgressStore
