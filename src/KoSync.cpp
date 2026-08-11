#include "KoSync.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <MD5Builder.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "BookSource.h"
#include "KoSyncStore.h"

namespace {
String deviceId() {
  uint64_t mac = ESP.getEfuseMac();
  char b[17];
  snprintf(b, sizeof(b), "%012llX", (unsigned long long)mac);
  return String(b);
}

void applyAuth(HTTPClient& http) {
  http.addHeader("Accept", "application/vnd.koreader.v1+json");
  http.addHeader("x-auth-user", KoSyncStore::user());
  http.addHeader("x-auth-key", KoSyncStore::key());
}

bool beginUrl(HTTPClient& http, WiFiClientSecure& sec, WiFiClient& plain,
              const String& url) {
  if (url.startsWith("https")) { sec.setInsecure(); return http.begin(sec, url); }
  return http.begin(plain, url);
}
}  // namespace

namespace KoSync {

String docHash(const String& path, bool fromSd) {
  BookSource s;
  if (!(fromSd ? s.openSD(path) : s.openLittleFS(path))) return "";
  uint32_t fileSize = s.size();
  MD5Builder md5;
  md5.begin();
  uint8_t buf[1024];
  for (int i = -1; i <= 10; ++i) {                 // KOReader partial MD5
    uint32_t offset = (i < 0) ? 0u : (1024u << (2 * i));
    if (offset >= fileSize) continue;
    uint32_t want = fileSize - offset;
    if (want > sizeof(buf)) want = sizeof(buf);
    s.seek(offset);
    size_t got = s.read((char*)buf, want);
    if (got) md5.add(buf, got);
  }
  s.close();
  md5.calculate();
  return md5.toString();
}

Result sync(const String& hash, float localPct, const String& title) {
  Result r;
  if (!KoSyncStore::has()) { r.msg = "No KOSync server set"; return r; }
  if (WiFi.status() != WL_CONNECTED) { r.msg = "Wi-Fi not connected"; return r; }
  if (hash.length() == 0) { r.msg = "Cannot hash this book"; return r; }
  if (localPct < 0) localPct = 0; if (localPct > 1) localPct = 1;
  String base = KoSyncStore::url();

  // --- pull remote ---
  {
    WiFiClientSecure sec; WiFiClient plain; HTTPClient http;
    if (beginUrl(http, sec, plain, base + "/syncs/progress/" + hash)) {
      http.setTimeout(12000);
      applyAuth(http);
      int code = http.GET();
      if (code == 200) {
        JsonDocument d;
        if (deserializeJson(d, http.getString()) == DeserializationError::Ok) {
          r.remotePct = d["percentage"].as<float>();
          r.haveRemote = r.remotePct > 0.0f;
        }
      } else if (code == 401) {
        http.end(); r.msg = "Auth failed (check user/password)"; return r;
      }
      http.end();
    }
  }

  // --- push local ---
  bool pushed = false;
  {
    WiFiClientSecure sec; WiFiClient plain; HTTPClient http;
    if (beginUrl(http, sec, plain, base + "/syncs/progress")) {
      http.setTimeout(12000);
      applyAuth(http);
      http.addHeader("Content-Type", "application/json");
      JsonDocument d;
      d["document"] = hash;
      d["progress"] = String(localPct, 6);
      d["percentage"] = localPct;
      d["device"] = "VixOS";
      d["device_id"] = deviceId();
      auto meta = d["metadata"].to<JsonObject>();
      meta["title"] = title;
      String body; serializeJson(d, body);
      int code = http.sendRequest("PUT", body);
      pushed = (code >= 200 && code < 300);
      if (code == 401) { http.end(); r.msg = "Auth failed (check user/password)"; return r; }
      http.end();
    }
  }

  r.ok = true;
  int lp = (int)(localPct * 100 + 0.5f);
  if (r.haveRemote && r.remotePct > localPct + 0.01f) {
    int rp = (int)(r.remotePct * 100 + 0.5f);
    r.msg = String("Remote is ahead: ") + rp + "%\nJumped to remote position.";
  } else if (pushed) {
    r.msg = String("Uploaded progress: ") + lp + "%";
    if (r.haveRemote) r.msg += String("\nRemote: ") + (int)(r.remotePct * 100 + 0.5f) + "%";
  } else {
    r.msg = "Sync reached server but upload was refused";
  }
  return r;
}

}  // namespace KoSync
