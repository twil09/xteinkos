#include "Clock.h"

#include <LittleFS.h>
#include <WiFi.h>
#include <time.h>

#include "config.h"

namespace {
int   g_offsetMin = 0;
bool  g_offsetLoaded = false;
bool  g_ntpStarted = false;

void loadOffset() {
  if (g_offsetLoaded) return;
  g_offsetLoaded = true;
  File f = LittleFS.open(TZ_PATH, "r");
  if (f) { g_offsetMin = f.readString().toInt(); f.close(); }
}

void saveOffset() {
  File f = LittleFS.open(TZ_PATH, "w");
  if (f) { f.print(g_offsetMin); f.close(); }
}

// Local time = UTC + offset. We keep the RTC in UTC and add the offset when
// formatting, so changing the zone never needs a re-sync.
bool localNow(struct tm& out) {
  time_t utc = time(nullptr);
  if (utc < 1000000000) return false;  // < 2001 => not synced yet
  loadOffset();
  time_t local = utc + (time_t)g_offsetMin * 60;
  gmtime_r(&local, &out);
  return true;
}
}  // namespace

namespace Clock {

void beginSync() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (g_ntpStarted) return;
  g_ntpStarted = true;
  // Keep the RTC in UTC; offset is applied at format time.
  configTime(0, 0, "pool.ntp.org", "time.nist.gov", "time.google.com");
}

bool synced() {
  struct tm t;
  return localNow(t);
}

String hhmm() {
  struct tm t;
  if (!localNow(t)) return "--:--";
  char buf[8];
  snprintf(buf, sizeof(buf), "%02d:%02d", t.tm_hour, t.tm_min);
  return String(buf);
}

String dateStr() {
  struct tm t;
  if (!localNow(t)) return "";
  static const char* wd[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  static const char* mo[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                             "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  char buf[20];
  snprintf(buf, sizeof(buf), "%s %d %s", wd[t.tm_wday % 7], t.tm_mday,
           mo[t.tm_mon % 12]);
  return String(buf);
}

int offsetMinutes() { loadOffset(); return g_offsetMin; }
int offsetHours()   { loadOffset(); return g_offsetMin / 60; }

void setOffsetHours(int hours) {
  if (hours < -12) hours = -12;
  if (hours > 14) hours = 14;
  loadOffset();
  g_offsetMin = hours * 60;
  saveOffset();
}

}  // namespace Clock
