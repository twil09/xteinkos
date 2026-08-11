#include "KoSyncStore.h"

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <MD5Builder.h>

#define KOSYNC_PATH "/kosync.json"

namespace {
String md5Hex(const String& s) {
  MD5Builder b;
  b.begin();
  b.add(s);
  b.calculate();
  return b.toString();
}
void read(JsonDocument& d) {
  File f = LittleFS.open(KOSYNC_PATH, "r");
  if (f) { deserializeJson(d, f); f.close(); }
}
}  // namespace

namespace KoSyncStore {

bool has() {
  JsonDocument d; read(d);
  return String((const char*)(d["url"] | "")).length() > 0 &&
         String((const char*)(d["user"] | "")).length() > 0;
}
String url()  { JsonDocument d; read(d); return String((const char*)(d["url"] | "")); }
String user() { JsonDocument d; read(d); return String((const char*)(d["user"] | "")); }
String key()  { JsonDocument d; read(d); return String((const char*)(d["key"] | "")); }

void save(const String& url, const String& user, const String& plainPassword) {
  String u = url; u.trim();
  while (u.endsWith("/")) u.remove(u.length() - 1);
  JsonDocument d;
  read(d);
  d["url"] = u;
  d["user"] = user;
  if (plainPassword.length()) d["key"] = md5Hex(plainPassword);  // keep old key if blank
  File f = LittleFS.open(KOSYNC_PATH, "w");
  if (f) { serializeJson(d, f); f.close(); }
}

void clear() { LittleFS.remove(KOSYNC_PATH); }

}  // namespace KoSyncStore
