// Stats — reading time, pages, completion, and a 1-100 reading-speed score.
#pragma once
#include <Arduino.h>
#include "config.h"

namespace Stats {
// Accumulate a reading session: dwell time (ms, capped by caller) + pages read.
void addSession(uint32_t ms, int pages);

uint32_t totalMinutes();
uint32_t totalPages();
// 0 = not enough data; else 1 (slow) .. 100 (fast) from average sec/page.
int speedScore();

void setCompleted(const String& key, bool done);
bool isCompleted(const String& key);
}  // namespace Stats
