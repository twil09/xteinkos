#include "portal_server.h"

#include <WiFi.h>

// ---------------------------------------------------------------------------
//  Embedded portal page (kept in sync with web/portal.html). Duet palette.
// ---------------------------------------------------------------------------
static const char PORTAL_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>X3 OSINT Configuration</title>
<style>
  :root{
    --bg:#F5F5F0; --text:#2C3E50; --accent:#3498DB; --accent-dark:#2980B9;
    --light:#ECF0F1; --border:#BDC3C7;
  }
  *{box-sizing:border-box}
  body{background:var(--bg);color:var(--text);
       font-family:'Segoe UI',Roboto,Helvetica,Arial,sans-serif;margin:0;padding:16px}
  .container{max-width:600px;margin:16px auto;background:#fff;padding:24px;
             border-radius:8px;box-shadow:0 2px 8px rgba(0,0,0,.1)}
  h1{margin:0 0 4px;font-size:1.6rem}
  p.sub{margin:0 0 20px;color:#7f8c8d}
  label{display:block;font-weight:600;margin:14px 0 4px;font-size:.95rem}
  input,select{width:100%;padding:11px;border:1px solid var(--border);
               border-radius:4px;font-size:1rem;background:#fff;color:var(--text)}
  .hint{font-size:.8rem;color:#95a5a6;margin-top:4px}
  button{width:100%;margin-top:22px;padding:14px;background:var(--accent);
         color:#fff;border:none;border-radius:4px;cursor:pointer;
         font-weight:700;font-size:1rem}
  button:hover{background:var(--accent-dark)}
  .foot{text-align:center;color:#95a5a6;font-size:.75rem;margin-top:16px}
</style>
</head>
<body>
  <div class="container">
    <h1>&#128269; X3 OSINT Toolkit</h1>
    <p class="sub">Configure your reconnaissance device</p>
    <form action="/connect" method="POST">
      <label>WiFi Network (to connect to)</label>
      <input type="text" name="ssid" placeholder="HomeWiFi" required
             autocomplete="off" autocapitalize="none">

      <label>WiFi Password</label>
      <input type="password" name="pwd" placeholder="password"
             autocomplete="off">

      <label>Reconnaissance Mode</label>
      <select name="mode" required>
        <option value="Passive Scan">Passive Scan (WiFi only)</option>
        <option value="OSINT Mode" selected>OSINT Mode (WiFi + lookups)</option>
        <option value="Active Scan">Active Scan (WiFi + BLE*)</option>
      </select>
      <div class="hint">*BLE reserved for a future release.</div>

      <label>Data Server URL (optional)</label>
      <input type="url" name="server_url" placeholder="http://192.168.1.100:5000">
      <div class="hint">Base URL of your enrichment server (GeoIP, etc.).</div>

      <label>Scan Interval (minutes)</label>
      <input type="number" name="scan_min" min="1" max="1440" value="5">

      <button type="submit">Save &amp; Start Scanning</button>
    </form>
    <div class="foot">X3 OSINT OS</div>
  </div>
</body>
</html>)HTML";

static const char SAVED_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head><meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Saved</title>
<style>body{background:#F5F5F0;color:#2C3E50;font-family:'Segoe UI',sans-serif;
text-align:center;padding:48px 16px}.c{max-width:480px;margin:auto;background:#fff;
padding:28px;border-radius:8px;box-shadow:0 2px 8px rgba(0,0,0,.1)}
h1{color:#27AE60}</style></head>
<body><div class="c"><h1>&#10004; Saved</h1>
<p>Configuration stored. The device will restart in a few seconds and connect to
your network.</p>
<p>You can close this page.</p></div></body></html>)HTML";

void PortalServer::begin(const char* ssid, const char* password) {
  configReceived_ = false;

  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP_, apIP_, IPAddress(255, 255, 255, 0));
  bool ok = (password && strlen(password) >= 8)
                ? WiFi.softAP(ssid, password)
                : WiFi.softAP(ssid);  // open AP if password too short/empty
  apIP_ = WiFi.softAPIP();

  // Route every DNS query to us so the OS opens the captive page.
  dns_.setErrorReplyCode(DNSReplyCode::NoError);
  dns_.start(AP_DNS_PORT, "*", apIP_);

  routes_();
  server_.begin();
  started_ = true;

  Serial.printf("[portal] AP '%s' %s, IP %s\n", ssid, ok ? "up" : "FAILED",
                apIP_.toString().c_str());
}

void PortalServer::routes_() {
  server_.on("/", HTTP_GET, [this]() { handleRoot_(); });
  server_.on("/connect", HTTP_POST, [this]() { handleConnect_(); });

  // Captive-portal probe endpoints across OSes -> bounce to the form.
  auto bounce = [this]() { redirectToRoot_(); };
  server_.on("/generate_204", bounce);           // Android
  server_.on("/gen_204", bounce);                // Android
  server_.on("/ncsi.txt", bounce);               // Windows
  server_.on("/connecttest.txt", bounce);        // Windows
  server_.on("/hotspot-detect.html", bounce);    // Apple
  server_.on("/library/test/success.html", bounce);
  server_.on("/fwlink", bounce);                 // Microsoft
  server_.onNotFound(bounce);
}

void PortalServer::handleRoot_() {
  server_.send_P(200, "text/html", PORTAL_HTML);
}

void PortalServer::redirectToRoot_() {
  String loc = "http://" + apIP_.toString() + "/";
  server_.sendHeader("Location", loc, true);
  server_.send(302, "text/plain", "");
}

void PortalServer::handleConnect_() {
  DeviceConfig cfg;
  cfg.ssid      = server_.arg("ssid");
  cfg.password  = server_.arg("pwd");
  cfg.serverUrl = server_.arg("server_url");
  cfg.mode      = parseMode(server_.arg("mode").c_str());

  long minutes = server_.arg("scan_min").toInt();
  if (minutes < 1) minutes = DEFAULT_SCAN_INTERVAL_S / 60;
  if (minutes > 1440) minutes = 1440;
  cfg.scanInterval = (uint32_t)(minutes * 60);
  cfg.configured = true;

  cfg.ssid.trim();
  cfg.serverUrl.trim();

  if (cfg.ssid.length() == 0) {
    server_.send(400, "text/plain", "SSID is required. Go back and try again.");
    return;
  }

  pending_ = cfg;
  configReceived_ = true;

  server_.send_P(200, "text/html", SAVED_HTML);
  Serial.printf("[portal] config received: ssid='%s' mode=%s interval=%us\n",
                cfg.ssid.c_str(), modeToken(cfg.mode), cfg.scanInterval);
}

void PortalServer::handle() {
  if (!started_) return;
  dns_.processNextRequest();
  server_.handleClient();
}

void PortalServer::getConfig(String& ssid, String& pwd, String& mode,
                             String& url) const {
  ssid = pending_.ssid;
  pwd  = pending_.password;
  mode = modeToken(pending_.mode);
  url  = pending_.serverUrl;
}
