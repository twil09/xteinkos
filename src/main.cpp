// ============================================================================
//  main.cpp — X3 OSINT OS orchestrator
// ----------------------------------------------------------------------------
//  Boot flow:
//    power/wake
//      -> mount FS + init display
//      -> force-portal button held OR no valid config?  -> captive portal
//      -> otherwise: connect home WiFi -> scan -> enrich -> render -> deep sleep
//
//  In scan mode all work happens in setup() and the device deep-sleeps at the
//  end; loop() only runs while the captive portal is active.
// ============================================================================
#include <Arduino.h>
#include <WiFi.h>
#include <esp_sleep.h>
#include <algorithm>
#include <vector>

#include "config.h"
#include "config_manager.h"
#include "display_driver.h"
#include "duet_theme.h"
#include "http_manager.h"
#include "osint_db.h"
#include "osint_logger.h"
#include "portal_server.h"
#include "wifi_scanner.h"

// ---------------------------------------------------------------------------
//  Globals
// ---------------------------------------------------------------------------
static ConfigManager configMgr;
static DeviceConfig  cfg;
static DisplayDriver display;
static DuetTheme     theme(display);
static PortalServer  portal;
static HTTPManager   http;
static OsintDB       db;
static OsintLogger   logger;
static WiFiScanner   scanner;

static bool portalMode = false;

// ---------------------------------------------------------------------------
//  Helpers
// ---------------------------------------------------------------------------
static void goDeepSleep(uint32_t seconds) {
  Serial.printf("[main] deep sleep %u s\n", seconds);
  display.hibernate();
  Serial.flush();
  esp_sleep_enable_timer_wakeup((uint64_t)seconds * 1000000ULL);
  esp_deep_sleep_start();  // never returns; setup() runs again on wake
}

static bool connectWiFi() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(true);
  WiFi.begin(cfg.ssid.c_str(), cfg.password.c_str());

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - start < WIFI_CONNECT_TIMEOUT_MS) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();
  bool ok = WiFi.status() == WL_CONNECTED;
  if (ok) Serial.printf("[wifi] connected, IP %s\n",
                        WiFi.localIP().toString().c_str());
  return ok;
}

static void fetchGeo(String& ip, String& country, String& city) {
  String base = cfg.serverUrl;
  while (base.endsWith("/")) base.remove(base.length() - 1);
  String url = base + "/api/geoip";

  JsonDocument doc;
  if (!http.fetchJSON(url.c_str(), doc)) {
    Serial.printf("[geo] fetch failed: %s\n", http.lastError().c_str());
    return;
  }
  if (!doc["ip"].isNull())            ip = doc["ip"].as<String>();
  else if (!doc["query"].isNull())    ip = doc["query"].as<String>();
  if (!doc["country"].isNull())       country = doc["country"].as<String>();
  else if (!doc["country_name"].isNull()) country = doc["country_name"].as<String>();
  if (!doc["city"].isNull())          city = doc["city"].as<String>();
}

static void startPortal() {
  portalMode = true;
  theme.showMessage("Waiting for config",
                    "1. Join WiFi:  " AP_SSID,
                    "2. Password:  " AP_PASSWORD,
                    "3. Open the setup page that appears.");
  portal.begin(AP_SSID, AP_PASSWORD);
  Serial.println(F("[main] captive portal active"));
}

static void runScanCycle() {
  db.begin();
  db.setHomeSsid(cfg.ssid);
  logger.begin();

  theme.showMessage("Connecting...", "Network: " + cfg.ssid,
                    "Mode: " + String(modeToken(cfg.mode)));

  if (!connectWiFi()) {
    theme.showMessage("WiFi failed", "Could not join " + cfg.ssid,
                      "Retrying in 30 seconds...");
    goDeepSleep(WIFI_RETRY_DELAY_MS / 1000);
    return;  // unreachable
  }

  logger.logSession("scan start");

  // ---- optional online enrichment (device-level GeoIP) ----
  //  Done first, while the STA link is freshly established — a WiFi scan can
  //  briefly perturb the connection on some cores.
  String ip, country, city;
  if (cfg.mode == OsintMode::OsintMode && cfg.serverUrl.length() > 0) {
    http.setConnectTimeout(HTTP_CONNECT_TIMEOUT_MS);
    http.setReadTimeout(HTTP_READ_TIMEOUT_MS);
    fetchGeo(ip, country, city);
  }

  // ---- scan + offline enrichment ----
  std::vector<ScannedAP> nets;
  scanner.scanEnriched(nets, db);

  // ---- log everything ----
  for (const auto& ap : nets) logger.logWifi(ap);

  // ---- strongest networks first ----
  std::sort(nets.begin(), nets.end(),
            [](const ScannedAP& a, const ScannedAP& b) {
              return a.rssi > b.rssi;
            });

  // ---- compose the frame ----
  theme.begin();
  theme.setHeader("X3 OSINT  -  " + String((int)nets.size()) + " networks");
  if (ip.length() || country.length() || city.length()) {
    theme.setGeoIP(ip, country, city);
  }
  for (const auto& ap : nets) theme.addWiFi(ap);

  String url, label;
  if (cfg.serverUrl.length() > 0) {
    url = cfg.serverUrl;
    label = "Open dashboard";
  } else {
    url = "https://github.com/twil09/xteinkos";
    label = "X3 OSINT OS";
  }
  theme.setQR(url, label);
  theme.setStatus("Next scan in " + String(cfg.scanInterval / 60) + " min",
                  false);
  theme.commit();

  // ---- power down radio + sleep ----
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  goDeepSleep(cfg.scanInterval);
}

// ---------------------------------------------------------------------------
//  Arduino entry points
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(50);
  Serial.printf("\n=== %s v%s ===\n", FW_NAME, FW_VERSION);

  pinMode(PIN_FORCE_PORTAL, INPUT_PULLUP);

  configMgr.begin();
  display.begin();

  bool forcePortal =
      (digitalRead(PIN_FORCE_PORTAL) == FORCE_PORTAL_ACTIVE_LEVEL);

  if (forcePortal || !configMgr.exists()) {
    if (forcePortal) Serial.println(F("[main] force-portal button held"));
    startPortal();
    return;  // loop() drives the portal
  }

  if (!configMgr.load(cfg)) {
    Serial.println(F("[main] config load failed -> portal"));
    startPortal();
    return;
  }

  runScanCycle();  // ends in deep sleep
}

void loop() {
  if (!portalMode) {
    // Scan mode never reaches here (setup() deep-sleeps). Guard anyway.
    delay(1000);
    return;
  }

  portal.handle();

  if (portal.getConfigReceived()) {
    portal.getConfig(cfg);
    bool saved = configMgr.save(cfg);
    if (saved) {
      theme.showMessage("Saved!", "Restarting in 3 seconds...",
                        "Will connect to: " + cfg.ssid);
    } else {
      theme.showMessage("Save failed", "Storage error.",
                        "Restarting portal...");
    }
    delay(3000);
    ESP.restart();
  }
}
