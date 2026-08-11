// KoSync — KOReader-compatible progress sync client (percentage).
// Computes the KOReader partial-MD5 document id, then PUTs local progress and
// GETs remote progress from a KOSync server. Interoperable for percentage sync.
#pragma once
#include <Arduino.h>

namespace KoSync {

String docHash(const String& path, bool fromSd);

struct Result {
  bool  ok = false;       // network/auth succeeded
  bool  haveRemote = false;
  float remotePct = 0;    // 0..1
  String msg;             // human-readable summary
};

// Push localPct (0..1) and pull remote for `hash`. Returns a summary.
Result sync(const String& hash, float localPct, const String& title);

}  // namespace KoSync
