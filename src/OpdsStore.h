// OpdsStore — saved OPDS catalog servers (name + URL), in LittleFS /opds.json.
// Seeded with a couple of public free-book catalogs on first run. Edited from
// the phone via the file-transfer web page (no on-device keyboard needed).
#pragma once
#include <Arduino.h>
#include <vector>

struct OpdsServer { String name; String url; };

namespace OpdsStore {
std::vector<OpdsServer> list();
void add(const String& name, const String& url);
void removeAt(int index);
}  // namespace OpdsStore
