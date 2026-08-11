// BookmarkStore — per-book bookmarks kept in LittleFS /bookmarks.json.
// A bookmark is a byte offset into the source plus a short summary snippet;
// offsets are re-pagination-stable, so they land correctly at any font size.
#pragma once
#include <Arduino.h>
#include <vector>

struct Bookmark {
  uint32_t offset;
  String   summary;
};

namespace BookmarkStore {
std::vector<Bookmark> list(const String& key);
void add(const String& key, uint32_t offset, const String& summary);
void removeAt(const String& key, int index);
bool hasAt(const String& key, uint32_t offset);  // a bookmark near this offset
}  // namespace BookmarkStore
