#include "OpdsStore.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

#define OPDS_PATH "/opds.json"

namespace {
void seedIfEmpty(JsonDocument& doc) {
  JsonArray a = doc["servers"].as<JsonArray>();
  if (!a.isNull() && a.size() > 0) return;
  a = doc["servers"].to<JsonArray>();
  JsonObject s1 = a.add<JsonObject>();
  s1["name"] = "Standard Ebooks";
  s1["url"] = "https://standardebooks.org/feeds/opds";
  JsonObject s2 = a.add<JsonObject>();
  s2["name"] = "Project Gutenberg";
  s2["url"] = "https://m.gutenberg.org/ebooks.opds/";
}
void readDoc(JsonDocument& doc) {
  File f = LittleFS.open(OPDS_PATH, "r");
  if (f) { deserializeJson(doc, f); f.close(); }
}
void writeDoc(JsonDocument& doc) {
  File f = LittleFS.open(OPDS_PATH, "w");
  if (f) { serializeJson(doc, f); f.close(); }
}
}  // namespace

namespace OpdsStore {

std::vector<OpdsServer> list() {
  JsonDocument doc;
  readDoc(doc);
  bool wasEmpty = doc["servers"].as<JsonArray>().isNull() ||
                  doc["servers"].as<JsonArray>().size() == 0;
  if (wasEmpty) { seedIfEmpty(doc); writeDoc(doc); }
  std::vector<OpdsServer> out;
  for (JsonObject o : doc["servers"].as<JsonArray>())
    out.push_back({String((const char*)(o["name"] | "")),
                   String((const char*)(o["url"] | ""))});
  return out;
}

void add(const String& name, const String& url) {
  if (url.length() == 0) return;
  JsonDocument doc;
  readDoc(doc);
  JsonArray a = doc["servers"].as<JsonArray>();
  if (a.isNull()) a = doc["servers"].to<JsonArray>();
  JsonObject o = a.add<JsonObject>();
  o["name"] = name.length() ? name : url;
  o["url"] = url;
  writeDoc(doc);
}

void removeAt(int index) {
  JsonDocument doc;
  readDoc(doc);
  JsonArray a = doc["servers"].as<JsonArray>();
  if (a.isNull() || index < 0 || index >= (int)a.size()) return;
  a.remove(index);
  writeDoc(doc);
}

}  // namespace OpdsStore
