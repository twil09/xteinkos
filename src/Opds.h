// Opds — fetch and parse OPDS (Atom) catalog feeds, and download book files.
// Used to browse public/free catalogs over Wi-Fi and pull EPUBs onto the device.
#pragma once
#include <Arduino.h>
#include <vector>

namespace Opds {

enum class EntryType { Nav, Book };
struct Entry {
  EntryType type;
  String    title;
  String    href;   // absolute URL
};
struct Feed {
  bool   ok = false;
  String error;
  String title;
  std::vector<Entry> entries;
  String nextUrl;    // absolute URL of the next page, or empty
};

Feed   fetch(const String& url);
// Download `url` to /books; savedPath receives the file path. Returns false on
// error (err set). `title` seeds the filename.
bool   download(const String& url, const String& title, String& savedPath, String& err);
String resolve(const String& base, const String& href);

}  // namespace Opds
