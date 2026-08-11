#include "BookmarkStore.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

#define BM_PATH "/bookmarks.json"

namespace {
void readDoc(JsonDocument& doc) {
  File f = LittleFS.open(BM_PATH, "r");
  if (!f) return;
  deserializeJson(doc, f);
  f.close();
}
void writeDoc(JsonDocument& doc) {
  File f = LittleFS.open(BM_PATH, "w");
  if (f) { serializeJson(doc, f); f.close(); }
}
}  // namespace

namespace BookmarkStore {

std::vector<Bookmark> list(const String& key) {
  std::vector<Bookmark> out;
  JsonDocument doc;
  readDoc(doc);
  JsonArray arr = doc[key].as<JsonArray>();
  if (!arr.isNull())
    for (JsonObject o : arr)
      out.push_back({(uint32_t)(o["o"] | 0), String((const char*)(o["s"] | ""))});
  return out;
}

void add(const String& key, uint32_t offset, const String& summary) {
  JsonDocument doc;
  readDoc(doc);
  JsonArray arr = doc[key].as<JsonArray>();  // existing, or...
  if (arr.isNull()) arr = doc[key].to<JsonArray>();  // ...create empty
  // avoid duplicates within ~200 bytes
  for (JsonObject o : arr)
    if (labs((long)((uint32_t)(o["o"] | 0)) - (long)offset) < 200) return;
  JsonObject e = arr.add<JsonObject>();
  e["o"] = offset;
  e["s"] = summary;
  writeDoc(doc);
}

void removeAt(const String& key, int index) {
  JsonDocument doc;
  readDoc(doc);
  JsonArray arr = doc[key].as<JsonArray>();
  if (arr.isNull() || index < 0 || index >= (int)arr.size()) return;
  arr.remove(index);
  writeDoc(doc);
}

bool hasAt(const String& key, uint32_t offset) {
  for (auto& b : list(key))
    if (labs((long)b.offset - (long)offset) < 200) return true;
  return false;
}

}  // namespace BookmarkStore
