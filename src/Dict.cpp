#include "Dict.h"

#include <LittleFS.h>
#include <SDCardManager.h>

#include "BookSource.h"

namespace {
bool   g_scanned = false;
bool   g_ok = false;
bool   g_sd = false;
String g_stem;   // full path without extension, e.g. /dict/webster
String g_name;

char lc(char c) { return (c >= 'A' && c <= 'Z') ? c + 32 : c; }

bool existsOn(bool sd, const String& path) {
  if (sd) {
    BookSource b;
    if (b.openSD(path)) { b.close(); return true; }
    return false;
  }
  return LittleFS.exists(path);
}

// Find "<stem>.idx" (+ .dict) under /dict on SD, else LittleFS.
void scan() {
  g_scanned = true;
  auto tryDir = [&](bool sd) -> bool {
    std::vector<String> files;
    if (sd) {
      if (!SdMan.ready()) return false;
      files = SdMan.listFiles("/dict", 60);
    } else {
      File d = LittleFS.open("/dict");
      if (d && d.isDirectory())
        for (File f = d.openNextFile(); f; f = d.openNextFile())
          if (!f.isDirectory()) files.push_back(String(f.name()));
    }
    for (auto& f : files) {
      String low = f; low.toLowerCase();
      if (!low.endsWith(".idx")) continue;
      int s = f.lastIndexOf('/');
      String base = (s >= 0) ? f.substring(s + 1) : f;      // name.idx
      String stem = String("/dict/") + base.substring(0, base.length() - 4);
      if (existsOn(sd, stem + ".idx") && existsOn(sd, stem + ".dict")) {
        g_ok = true; g_sd = sd; g_stem = stem;
        int ns = stem.lastIndexOf('/');
        g_name = ns >= 0 ? stem.substring(ns + 1) : stem;
        return true;
      }
    }
    return false;
  };
  SdMan.begin();
  if (tryDir(true)) return;
  tryDir(false);
}

struct Reader {
  BookSource s;
  uint8_t buf[2048];
  size_t len = 0, pos = 0;
  bool eof = false;
  bool open(bool sd, const String& p) { return sd ? s.openSD(p) : s.openLittleFS(p); }
  int get() {
    if (pos >= len) {
      len = s.read((char*)buf, sizeof(buf));
      pos = 0;
      if (!len) { eof = true; return -1; }
    }
    return buf[pos++];
  }
  void close() { s.close(); }
};

void stripHtml(String& in) {
  if (in.indexOf('<') < 0) return;
  String o; bool tag = false;
  for (size_t i = 0; i < in.length(); ++i) {
    char c = in[i];
    if (c == '<') tag = true;
    else if (c == '>') tag = false;
    else if (!tag) o += c;
  }
  o.replace("&amp;", "&"); o.replace("&lt;", "<"); o.replace("&gt;", ">");
  o.replace("&quot;", "\""); o.replace("&#39;", "'");
  in = o;
}
}  // namespace

namespace Dict {

bool available() {
  if (!g_scanned) scan();
  return g_ok;
}

String name() { return g_name; }

bool lookup(const String& word, String& defOut) {
  if (!available()) return false;
  String target = word; target.toLowerCase(); target.trim();
  if (target.length() == 0) return false;

  Reader idx;
  if (!idx.open(g_sd, g_stem + ".idx")) return false;

  char w[160];
  bool found = false;
  uint32_t offset = 0, size = 0;
  while (!idx.eof) {
    size_t wl = 0;
    int c;
    while ((c = idx.get()) > 0) { if (wl < sizeof(w) - 1) w[wl++] = (char)c; }
    if (c < 0) break;         // EOF mid-word
    w[wl] = 0;
    // read 8 bytes: offset(4 BE) + size(4 BE)
    int b[8]; bool ok = true;
    for (int i = 0; i < 8; ++i) { b[i] = idx.get(); if (b[i] < 0) { ok = false; break; } }
    if (!ok) break;
    offset = ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | b[3];
    size   = ((uint32_t)b[4] << 24) | ((uint32_t)b[5] << 16) | ((uint32_t)b[6] << 8) | b[7];
    // case-insensitive compare
    bool eq = ((size_t)wl == target.length());
    for (size_t i = 0; eq && i < wl; ++i) if (lc(w[i]) != target[i]) eq = false;
    if (eq) { found = true; break; }
  }
  idx.close();
  if (!found) return false;
  if (size == 0 || size > 32768) return false;

  BookSource dict;
  if (!(g_sd ? dict.openSD(g_stem + ".dict") : dict.openLittleFS(g_stem + ".dict")))
    return false;
  dict.seek(offset);
  char* d = (char*)malloc(size + 1);
  if (!d) { dict.close(); return false; }
  size_t got = dict.read(d, size);
  d[got] = 0;
  dict.close();
  defOut = String(d);
  free(d);
  stripHtml(defOut);
  defOut.trim();
  return defOut.length() > 0;
}

}  // namespace Dict
