// Clock — wall-clock time via NTP (set when Wi-Fi connects) plus a user UTC
// offset. The ESP32 RTC keeps running through deep sleep, so once synced the
// time survives naps; a full power-off loses it until the next NTP sync.
#pragma once
#include <Arduino.h>

namespace Clock {

// Kick an NTP sync (safe to call repeatedly; only acts when Wi-Fi is up).
void beginSync();

// True once we've obtained a real time from NTP this power cycle.
bool synced();

// "14:05" (24h) or "--:--" before the first sync.
String hhmm();

// "Mon 11 Aug" or "" before the first sync.
String dateStr();

// User time-zone offset from UTC, persisted to LittleFS.
int  offsetMinutes();
void setOffsetHours(int hours);   // clamps to [-12, +14]
int  offsetHours();

}  // namespace Clock
