#include "Net.h"

#include <ESPmDNS.h>
#include <LittleFS.h>
#include <WebServer.h>
#include <WiFi.h>

#include "Clock.h"
#include "KoSyncStore.h"
#include "OpdsStore.h"
#include "WifiStore.h"
#include "config.h"

namespace {
WebServer server(FILE_PORT);
bool  g_serverRunning = false;
bool  g_mdnsUp = false;
bool  g_wasConnected = false;

const char* kBookDir  = "/books";
const char* kImageDir = "/images";

bool isImageName(const String& lower) {
  const char* e[] = {".jpg", ".jpeg", ".png", ".gif", ".bmp", ".cbz", ".cbr"};
  for (auto x : e) if (lower.endsWith(x)) return true;
  return false;
}

String basename(const String& p) {
  int s = p.lastIndexOf('/');
  int b = p.lastIndexOf('\\');
  int i = max(s, b);
  return (i >= 0) ? p.substring(i + 1) : p;
}

String htmlEscape(const String& s) {
  String o;
  for (size_t i = 0; i < s.length(); ++i) {
    char c = s[i];
    if (c == '&') o += "&amp;";
    else if (c == '<') o += "&lt;";
    else if (c == '>') o += "&gt;";
    else if (c == '"') o += "&quot;";
    else o += c;
  }
  return o;
}

// One <li> row per file in dir, with download + delete links.
void listDir(String& out, const char* dir) {
  File d = LittleFS.open(dir);
  if (!d || !d.isDirectory()) return;
  for (File f = d.openNextFile(); f; f = d.openNextFile()) {
    if (f.isDirectory()) continue;
    String full = f.name();
    if (full[0] != '/') full = String(dir) + "/" + full;
    String name = basename(full);
    out += "<li><span>" + htmlEscape(name) + "</span> <small>" +
           String((uint32_t)f.size() / 1024) + " KB</small> "
           "<a href=\"/dl?p=" + full + "\">get</a> "
           "<a class=del href=\"/rm?p=" + full + "\">delete</a></li>";
  }
}

void handleRoot() {
  size_t total = LittleFS.totalBytes(), used = LittleFS.usedBytes();
  String h;
  h.reserve(4096);
  h += "<!doctype html><html><head><meta charset=UTF-8>"
       "<meta name=viewport content=\"width=device-width,initial-scale=1\">"
       "<title>Vix OS \xE2\x80\x94 Files</title><style>"
       "body{font-family:'Segoe UI',system-ui,sans-serif;background:" DUET_RGB_BG
       ";color:" DUET_RGB_TEXT ";margin:0;padding:16px}"
       ".c{max-width:520px;margin:auto}h1{font-size:1.4rem}"
       ".card{background:#fff;border-radius:10px;padding:18px;margin:14px 0;"
       "box-shadow:0 2px 8px rgba(0,0,0,.08)}"
       "ul{list-style:none;padding:0;margin:0}li{display:flex;align-items:center;"
       "gap:8px;padding:8px 0;border-bottom:1px solid #eee;font-size:.95rem}"
       "li span{flex:1;word-break:break-all}small{color:#888}"
       "a{color:" DUET_RGB_ACCENT ";text-decoration:none;font-weight:600}"
       "a.del{color:#c0392b}"
       "input[type=file]{width:100%;margin:10px 0}"
       "button{width:100%;padding:13px;background:" DUET_RGB_ACCENT ";color:#fff;"
       "border:0;border-radius:6px;font-weight:700;font-size:1rem}"
       ".bar{height:8px;background:#e6e6e6;border-radius:4px;overflow:hidden}"
       ".bar>i{display:block;height:100%;background:" DUET_RGB_ACCENT "}"
       "</style></head><body><div class=c>";
  h += "<h1>&#128218; Vix OS files</h1>";
  int pct = total ? (int)(used * 100 / total) : 0;
  h += "<div class=card><b>Storage</b><div class=bar><i style=\"width:" +
       String(pct) + "%\"></i></div><small>" + String((uint32_t)used / 1024) +
       " KB of " + String((uint32_t)total / 1024) + " KB used</small></div>";

  h += "<div class=card><b>Upload</b>"
       "<form method=POST action=\"/upload\" enctype=\"multipart/form-data\">"
       "<input type=file name=f multiple>"
       "<button type=submit>Send to Vix OS</button></form>"
       "<small>Books go to the Library; images/comics to Images "
       "(sorted by file type).</small></div>";

  h += "<div class=card><b>Books</b><ul>";
  listDir(h, kBookDir);
  h += "</ul></div><div class=card><b>Images</b><ul>";
  listDir(h, kImageDir);
  h += "</ul></div>";

  // OPDS catalog servers
  h += "<div class=card><b>OPDS catalogs</b><ul>";
  {
    auto servers = OpdsStore::list();
    for (size_t i = 0; i < servers.size(); ++i)
      h += "<li><span>" + htmlEscape(servers[i].name) + "</span>"
           "<a class=del href=\"/opdsdel?i=" + String((int)i) + "\">delete</a></li>";
  }
  h += "</ul><form method=GET action=\"/opdsadd\">"
       "<input name=name placeholder=\"Name\"> <input name=url placeholder=\"https://.../opds\">"
       "<button type=submit>Add catalog</button></form></div>";

  // KOReader sync
  h += "<div class=card><b>KOReader sync</b>";
  if (KoSyncStore::has())
    h += "<small>Configured: " + htmlEscape(KoSyncStore::user()) + " @ " +
         htmlEscape(KoSyncStore::url()) + "</small>";
  h += "<form method=GET action=\"/kosync\">"
       "<input name=url placeholder=\"https://sync.server\"> "
       "<input name=user placeholder=\"username\"> "
       "<input name=pwd type=password placeholder=\"password\">"
       "<button type=submit>Save sync server</button></form></div>";

  h += "</div></body></html>";
  server.send(200, "text/html", h);
}

File g_upFile;
void handleUpload() {
  HTTPUpload& u = server.upload();
  if (u.status == UPLOAD_FILE_START) {
    LittleFS.mkdir(kBookDir);
    LittleFS.mkdir(kImageDir);
    String name = basename(u.filename);
    name.replace("\"", "");
    String lower = name; lower.toLowerCase();
    String dir = isImageName(lower) ? kImageDir : kBookDir;
    String path = dir + "/" + name;
    g_upFile = LittleFS.open(path, "w");
  } else if (u.status == UPLOAD_FILE_WRITE) {
    if (g_upFile) g_upFile.write(u.buf, u.currentSize);
  } else if (u.status == UPLOAD_FILE_END) {
    if (g_upFile) g_upFile.close();
  }
}

void handleDownload() {
  if (!server.hasArg("p")) { server.send(400, "text/plain", "missing p"); return; }
  String p = server.arg("p");
  File f = LittleFS.open(p, "r");
  if (!f || f.isDirectory()) { server.send(404, "text/plain", "not found"); return; }
  String lower = p; lower.toLowerCase();
  String ct = lower.endsWith(".txt") || lower.endsWith(".md")
                  ? "text/plain"
                  : "application/octet-stream";
  server.sendHeader("Content-Disposition",
                    "attachment; filename=\"" + basename(p) + "\"");
  server.streamFile(f, ct);
  f.close();
}

void handleDelete() {
  if (server.hasArg("p")) LittleFS.remove(server.arg("p"));
  server.sendHeader("Location", "/");
  server.send(303, "text/plain", "");
}

void ensureMdns() {
  if (g_mdnsUp) return;
  if (MDNS.begin(MDNS_HOST)) {
    MDNS.addService("http", "tcp", FILE_PORT);
    g_mdnsUp = true;
  }
}
}  // namespace

namespace Net {

void connectSaved() {
  String s, p;
  if (!WifiStore::load(s, p)) return;
  WiFi.mode(WIFI_STA);
  WiFi.begin(s.c_str(), p.c_str());
}

void begin() {
  WiFi.persistent(true);
  WiFi.setAutoReconnect(true);
  WiFi.mode(WIFI_STA);
  connectSaved();
}

bool connected() { return WiFi.status() == WL_CONNECTED; }
String ssid() { return WiFi.SSID(); }
String ip() { return WiFi.localIP().toString(); }

String url() { return "http://" + ip() + "/"; }  // IP: resolves on any phone
String hostUrl() { return String("http://" MDNS_HOST ".local/"); }

void poll() {
  bool up = connected();
  if (up && !g_wasConnected) {   // link just came up
    ensureMdns();
    Clock::beginSync();
  }
  if (!up) g_mdnsUp = false;     // will re-announce on reconnect
  g_wasConnected = up;
  if (g_serverRunning) server.handleClient();
}

void startFileServer() {
  if (g_serverRunning) return;
  LittleFS.mkdir(kBookDir);
  LittleFS.mkdir(kImageDir);
  ensureMdns();
  server.on("/", HTTP_GET, handleRoot);
  server.on("/upload", HTTP_POST, []() {
    server.sendHeader("Location", "/");
    server.send(303, "text/plain", "");
  }, handleUpload);
  server.on("/dl", HTTP_GET, handleDownload);
  server.on("/rm", HTTP_GET, handleDelete);
  server.on("/opdsadd", HTTP_GET, []() {
    OpdsStore::add(server.arg("name"), server.arg("url"));
    server.sendHeader("Location", "/");
    server.send(303, "text/plain", "");
  });
  server.on("/opdsdel", HTTP_GET, []() {
    OpdsStore::removeAt(server.arg("i").toInt());
    server.sendHeader("Location", "/");
    server.send(303, "text/plain", "");
  });
  server.on("/kosync", HTTP_GET, []() {
    KoSyncStore::save(server.arg("url"), server.arg("user"), server.arg("pwd"));
    server.sendHeader("Location", "/");
    server.send(303, "text/plain", "");
  });
  server.begin();
  g_serverRunning = true;
}

void stopFileServer() {
  if (!g_serverRunning) return;
  server.stop();
  g_serverRunning = false;
}

bool fileServerRunning() { return g_serverRunning; }

}  // namespace Net
