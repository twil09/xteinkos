// KoSyncStore — KOReader sync server credentials (LittleFS /kosync.json).
// The password is stored only as its MD5 (the KOSync "x-auth-key"), never plain.
// Configured from the phone via the file-transfer web page.
#pragma once
#include <Arduino.h>

namespace KoSyncStore {
bool   has();
String url();
String user();
String key();   // md5(password), the x-auth-key
void   save(const String& url, const String& user, const String& plainPassword);
void   clear();
}  // namespace KoSyncStore
