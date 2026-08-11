#include "Opds.h"

#include <HTTPClient.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <string.h>

namespace {

// Extract attribute value (name="...") between [from,to) of s.
String attr(const String& s, int from, int to, const char* name) {
  int a = s.indexOf(name, from);
  if (a < 0 || a >= to) return "";
  a += strlen(name);
  while (a < to && (s[a] == ' ' || s[a] == '=')) ++a;
  if (a >= to) return "";
  char q = s[a];
  if (q != '"' && q != '\'') return "";
  int b = s.indexOf(q, a + 1);
  if (b < 0 || b > to) return "";
  return s.substring(a + 1, b);
}

String tagText(const String& s, int from, int to, const char* tag) {
  int t = s.indexOf(tag, from);
  if (t < 0 || t >= to) return "";
  int g = s.indexOf('>', t);
  if (g < 0 || g >= to) return "";
  int e = s.indexOf('<', g + 1);
  if (e < 0 || e > to) return "";
  String v = s.substring(g + 1, e);
  v.trim();
  return v;
}

void decodeXmlEntities(String& s) {
  s.replace("&amp;", "&");
  s.replace("&lt;", "<");
  s.replace("&gt;", ">");
  s.replace("&quot;", "\"");
  s.replace("&#39;", "'");
  s.replace("&apos;", "'");
}

bool httpBegin(HTTPClient& http, WiFiClientSecure& sec, WiFiClient& plain,
               const String& url) {
  if (url.startsWith("https")) { sec.setInsecure(); return http.begin(sec, url); }
  return http.begin(plain, url);
}

String sanitizeName(const String& in) {
  String o;
  for (size_t i = 0; i < in.length() && o.length() < 60; ++i) {
    char c = in[i];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
        c == ' ' || c == '-' || c == '_' || c == '.')
      o += c;
    else if (c == ':' || c == '/') o += '-';
  }
  o.trim();
  if (o.length() == 0) o = "book";
  return o;
}

}  // namespace

namespace Opds {

String resolve(const String& base, const String& href) {
  if (href.startsWith("http://") || href.startsWith("https://")) return href;
  int sch = base.indexOf("://");
  if (sch < 0) return href;
  int authEnd = base.indexOf('/', sch + 3);
  String origin = authEnd < 0 ? base : base.substring(0, authEnd);
  if (href.startsWith("/")) return origin + href;
  // relative to base directory
  int lastSlash = base.lastIndexOf('/');
  String dir = (lastSlash > sch + 2) ? base.substring(0, lastSlash) : origin;
  return dir + "/" + href;
}

Feed fetch(const String& url) {
  Feed f;
  if (WiFi.status() != WL_CONNECTED) { f.error = "Wi-Fi not connected"; return f; }
  WiFiClientSecure sec;
  WiFiClient plain;
  HTTPClient http;
  if (!httpBegin(http, sec, plain, url)) { f.error = "Bad URL"; return f; }
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(15000);
  http.addHeader("User-Agent", "VixOS/1.0");
  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    f.error = String("HTTP ") + code;
    http.end();
    return f;
  }
  String body = http.getString();  // feeds are small; paginated
  http.end();

  int firstEntry = body.indexOf("<entry");
  f.title = tagText(body, 0, firstEntry < 0 ? body.length() : firstEntry, "<title");
  decodeXmlEntities(f.title);

  // next-page link (feed level): a <link rel="next" ...>
  {
    int p = 0;
    while (true) {
      int l = body.indexOf("<link", p);
      if (l < 0) break;
      int end = body.indexOf('>', l);
      if (end < 0) break;
      String rel = attr(body, l, end, "rel=");
      if (rel == "next") {
        f.nextUrl = resolve(url, attr(body, l, end, "href="));
        break;
      }
      p = end + 1;
    }
  }

  // entries
  int p = 0;
  while (f.entries.size() < 80) {
    int e0 = body.indexOf("<entry", p);
    if (e0 < 0) break;
    int e1 = body.indexOf("</entry>", e0);
    if (e1 < 0) e1 = body.length();
    Entry en;
    en.title = tagText(body, e0, e1, "<title");
    decodeXmlEntities(en.title);
    // scan links inside the entry
    String bookHref, navHref;
    int lp = e0;
    while (lp < e1) {
      int l = body.indexOf("<link", lp);
      if (l < 0 || l >= e1) break;
      int end = body.indexOf('>', l);
      if (end < 0 || end > e1) break;
      String type = attr(body, l, end, "type=");
      String rel = attr(body, l, end, "rel=");
      String href = attr(body, l, end, "href=");
      if (type.indexOf("epub") >= 0 ||
          (rel.indexOf("acquisition") >= 0 && bookHref.length() == 0))
        bookHref = href;
      else if (type.indexOf("atom+xml") >= 0 && navHref.length() == 0)
        navHref = href;
      lp = end + 1;
    }
    if (bookHref.length()) { en.type = EntryType::Book; en.href = resolve(url, bookHref); }
    else if (navHref.length()) { en.type = EntryType::Nav; en.href = resolve(url, navHref); }
    else { p = e1 + 1; continue; }
    if (en.title.length() == 0) en.title = "(untitled)";
    f.entries.push_back(en);
    p = e1 + 8;
  }

  f.ok = true;
  return f;
}

bool download(const String& url, const String& title, String& savedPath, String& err) {
  if (WiFi.status() != WL_CONNECTED) { err = "Wi-Fi not connected"; return false; }
  LittleFS.mkdir("/books");
  savedPath = String("/books/") + sanitizeName(title);
  if (!savedPath.endsWith(".epub")) savedPath += ".epub";

  WiFiClientSecure sec;
  WiFiClient plain;
  HTTPClient http;
  if (!httpBegin(http, sec, plain, url)) { err = "Bad URL"; return false; }
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(20000);
  http.addHeader("User-Agent", "VixOS/1.0");
  int code = http.GET();
  if (code != HTTP_CODE_OK) { err = String("HTTP ") + code; http.end(); return false; }

  int len = http.getSize();  // may be -1 (chunked)
  size_t freeBytes = LittleFS.totalBytes() - LittleFS.usedBytes();
  if (len > 0 && (size_t)len + 8192 > freeBytes) {
    err = "Too big for flash (use SD)";
    http.end();
    return false;
  }

  File out = LittleFS.open(savedPath, "w");
  if (!out) { err = "Cannot write file"; http.end(); return false; }
  WiFiClient* stream = http.getStreamPtr();
  uint8_t buf[1024];
  uint32_t total = 0;
  uint32_t idleStart = millis();
  while (http.connected() && (len < 0 || total < (uint32_t)len)) {
    size_t avail = stream->available();
    if (avail) {
      int r = stream->readBytes(buf, avail > sizeof(buf) ? sizeof(buf) : avail);
      if (r > 0) {
        out.write(buf, r);
        total += r;
        idleStart = millis();
        if (total + 8192 > freeBytes) { err = "Ran out of flash space"; out.close();
          LittleFS.remove(savedPath); http.end(); return false; }
      }
    } else {
      if (millis() - idleStart > 15000) break;  // stall timeout
      delay(5);
    }
  }
  out.close();
  http.end();
  if (total == 0) { LittleFS.remove(savedPath); err = "Empty download"; return false; }
  return true;
}

}  // namespace Opds
