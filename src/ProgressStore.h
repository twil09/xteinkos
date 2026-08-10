// ProgressStore — persists per-book reading position to LittleFS (JSON).
#pragma once
#include <Arduino.h>
#include "config.h"

namespace ProgressStore {
uint32_t load(const String& path);       // byte offset (0 if none)
void save(const String& path, uint32_t offset);
}  // namespace ProgressStore
