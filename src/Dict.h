// Dict — minimal StarDict lookup. Looks for an uncompressed dictionary under
// /dict on the SD card (or LittleFS): <stem>.ifo + <stem>.idx + <stem>.dict.
// Linear-scans the .idx for a case-insensitive exact match (32-bit offsets).
// Compressed .idx/.dict.dz are not supported (provide uncompressed files).
#pragma once
#include <Arduino.h>

namespace Dict {
bool   available();                 // true if a usable dictionary was found
String name();                      // dictionary book name / stem
bool   lookup(const String& word, String& defOut);
}  // namespace Dict
