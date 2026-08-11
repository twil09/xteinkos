// Epub — turn an .epub (a ZIP of XHTML) into a single plain-text file that the
// normal reader paginates. Handles container.xml -> OPF -> spine order, streams
// each chapter through miniz inflate + an XHTML tag stripper, and caches the
// result so re-opening the same book is instant.
#pragma once
#include <Arduino.h>

namespace Epub {

struct Result {
  bool   ok = false;
  String textPath;   // LittleFS path to the extracted plain text
  String title;      // book title from the OPF (may be empty)
  String error;      // human-readable reason when !ok
};

// Extract (or reuse a cached extraction of) the book. `srcPath`/`fromSd` locate
// the .epub. The returned textPath is always on LittleFS.
Result prepare(const String& srcPath, bool fromSd);

}  // namespace Epub
