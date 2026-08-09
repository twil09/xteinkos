// ============================================================================
//  main.cpp — X3 OSINT OS orchestrator (interactive)
// ----------------------------------------------------------------------------
//  Boot:
//    * hold BACK at power-on, or no valid config -> captive portal
//    * timer wake -> rescan, then interactive results
//    * power-on / button wake -> show cached results (fast), interactive
//
//  Interactive UI (button driven):
//    RESULTS  Up/Down move   Select -> DETAIL   Back -> MENU
//    DETAIL   Up/Down prev/next   Back -> RESULTS
//    MENU     Rescan / Reconfigure / Toggle GeoIP / Sleep / About
//    Power long-press -> deep sleep (wakes on power button or scan timer)
//    Inactivity -> deep sleep
// ============================================================================
#include <Arduino.h>
#include <WiFi.h>
#include <esp_sleep.h>
#include <driver/gpio.h>
#include <algorithm>
#include <vector>

#include "config.h"
#include "config_manager.h"
#include "display_driver.h"
#include "duet_theme.h"
#include "http_manager.h"
#include "input_buttons.h"
#include "osint_db.h"
#include "osint_logger.h"
#include "portal_server.h"
#include "results_store.h"
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
static InputButtons  buttons;
static ResultsStore  store;

enum class App : uint8_t { Portal, Results, Detail, Menu, About };
static App app = App::Results;

static std::vector<ScannedAP> nets;
static String   geoIp, geoCountry, geoCity;
static int      sel = 0, scrollTop = 0, menuSel = 0;
static uint32_t lastActivity = 0;

static const char* kMenuItems[] = {"Rescan now", "Reconfigure WiFi",
                                   "Toggle GeoIP mode", "Sleep now", "About"};
static const int kMenuCount = 5;

// ---------------------------------------------------------------------------
//  WiFi + enrichment
// ---------------------------------------------------------------------------
static bool connectWiFi() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(cfg.ssid.c_str(), cfg.password.c_str());
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - start < WIFI_CONNECT_TIMEOUT_MS) {
    delay(200);
  }
  return WiFi.status() == WL_CONNECTED;
}

static void fetchGeo() {
  geoIp = geoCountry = geoCity = "";
  String base = cfg.serverUrl;
  while (base.endsWith("/")) base.remove(base.length() - 1);
  String url = base + "/api/geoip";
  JsonDocument doc;
  if (!http.fetchJSON(url.c_str(), doc)) return;
  if (!doc["ip"].isNull()) geoIp = doc["ip"].as<String>();
  else if (!doc["query"].isNull()) geoIp = doc["query"].as<String>();
  if (!doc["country"].isNull()) geoCountry = doc["country"].as<String>();
  else if (!doc["country_name"].isNull()) geoCountry = doc["country_name"].as<String>();
  if (!doc["city"].isNull()) geoCity = doc["city"].as<String>();
}

// Full scan cycle: connect, (geo), scan, enrich, log, cache. Leaves radio off.
static void doScan() {
  theme.showMessage("Scanning...", "Connecting to " + cfg.ssid,
                    "Mode: " + String(modeToken(cfg.mode)));
  if (!connectWiFi()) {
    theme.showMessage("WiFi failed", "Could not join " + cfg.ssid,
                      "Showing last results / menu.");
    delay(1200);
    // Fall back to whatever cache we have.
    store.load(nets, geoIp, geoCountry, geoCity);
    sel = scrollTop = 0;
    return;
  }

  logger.logSession("scan start");
  if (cfg.mode == OsintMode::OsintMode && cfg.serverUrl.length() > 0) {
    http.setConnectTimeout(HTTP_CONNECT_TIMEOUT_MS);
    http.setReadTimeout(HTTP_READ_TIMEOUT_MS);
    fetchGeo();
  }

  scanner.scanEnriched(nets, db);
  for (const auto& ap : nets) logger.logWifi(ap);
  std::sort(nets.begin(), nets.end(),
            [](const ScannedAP& a, const ScannedAP& b) { return a.rssi > b.rssi; });

  store.save(nets, geoIp, geoCountry, geoCity);

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  sel = scrollTop = 0;
}

// ---------------------------------------------------------------------------
//  Rendering helpers
// ---------------------------------------------------------------------------
static void renderResults() {
  theme.showResults(nets, sel, scrollTop, geoIp, geoCountry, geoCity);
}

static void renderDetail() {
  if (nets.empty()) { app = App::Results; renderResults(); return; }
  const ScannedAP& ap = nets[sel];
  String qr, label;
  if (cfg.serverUrl.length() > 0) {
    qr = cfg.serverUrl + "/ap/" + ap.bssidStr;
    label = "Open in dashboard";
  } else {
    qr = "WIFI:S:" + ap.ssid + ";;";  // standard WiFi QR (SSID only)
    label = "Scan (SSID)";
  }
  theme.showDetail(ap, qr, label);
}

static void renderMenu() {
  std::vector<String> items;
  for (int i = 0; i < kMenuCount; ++i) items.push_back(kMenuItems[i]);
  items[2] = String("GeoIP mode: ") +
             (cfg.mode == OsintMode::OsintMode ? "ON" : "OFF");
  theme.showMenu("Menu", items, menuSel);
}

static void renderAbout() {
  theme.showMessage(String(FW_NAME) + " v" + FW_VERSION,
                    "Networks cached: " + String((int)nets.size()),
                    "SSID: " + cfg.ssid + "   Mode: " + modeToken(cfg.mode),
                    "Passive WiFi recon. Back to return.");
}

static void renderCurrent() {
  switch (app) {
    case App::Results: renderResults(); break;
    case App::Detail:  renderDetail();  break;
    case App::Menu:    renderMenu();    break;
    case App::About:   renderAbout();   break;
    default: break;
  }
}

// Keep the selected row within the visible window.
static void fixScroll() {
  int rows = theme.rowsPerPage();
  if (sel < 0) sel = 0;
  if (sel >= (int)nets.size()) sel = (int)nets.size() - 1;
  if (sel < 0) sel = 0;
  if (sel < scrollTop) scrollTop = sel;
  if (sel >= scrollTop + rows) scrollTop = sel - rows + 1;
  if (scrollTop < 0) scrollTop = 0;
}

// ---------------------------------------------------------------------------
//  Sleep / wake
// ---------------------------------------------------------------------------
static void deepSleep() {
  // Wait for the power button to be released so we don't instantly re-wake.
  uint32_t t = millis();
  while (digitalRead(BTN_POWER_PIN) == LOW && millis() - t < 3000) delay(20);

  display.hibernate();  // keep the current image on screen
  Serial.flush();

  gpio_pullup_en((gpio_num_t)PIN_WAKE);
  gpio_pulldown_dis((gpio_num_t)PIN_WAKE);
  esp_deep_sleep_enable_gpio_wakeup(1ULL << PIN_WAKE, ESP_GPIO_WAKEUP_GPIO_LOW);
  esp_sleep_enable_timer_wakeup((uint64_t)cfg.scanInterval * 1000000ULL);
  esp_deep_sleep_start();  // never returns
}

// ---------------------------------------------------------------------------
//  Captive portal
// ---------------------------------------------------------------------------
static void startPortal() {
  app = App::Portal;
  theme.showMessage("Waiting for config", "1. Join WiFi:  " AP_SSID,
                    "2. Password:  " AP_PASSWORD,
                    "3. Open the setup page that appears.");
  portal.begin(AP_SSID, AP_PASSWORD);
}

// ---------------------------------------------------------------------------
//  Button handling per screen
// ---------------------------------------------------------------------------
static void doMenuAction(int idx) {
  switch (idx) {
    case 0:  // Rescan now
      doScan();
      app = App::Results;
      renderResults();
      break;
    case 1:  // Reconfigure WiFi
      theme.showMessage("Reconfigure", "Clearing settings...",
                        "Restarting into setup portal.");
      configMgr.clear();
      delay(1500);
      ESP.restart();
      break;
    case 2:  // Toggle GeoIP mode
      cfg.mode = (cfg.mode == OsintMode::OsintMode) ? OsintMode::PassiveScan
                                                    : OsintMode::OsintMode;
      configMgr.save(cfg);
      renderMenu();
      break;
    case 3:  // Sleep now
      deepSleep();
      break;
    case 4:  // About
      app = App::About;
      renderAbout();
      break;
  }
}

static void handleButton(Btn e) {
  switch (app) {
    case App::Results:
      if (e == Btn::Up)   { sel--; fixScroll(); renderResults(); }
      else if (e == Btn::Down) { sel++; fixScroll(); renderResults(); }
      else if (e == Btn::Select) {
        if (nets.empty()) { doScan(); renderResults(); }
        else { app = App::Detail; renderDetail(); }
      } else if (e == Btn::Back) { app = App::Menu; menuSel = 0; renderMenu(); }
      break;

    case App::Detail:
      if (e == Btn::Back) { app = App::Results; renderResults(); }
      else if (e == Btn::Up)   { sel--; fixScroll(); renderDetail(); }
      else if (e == Btn::Down) { sel++; fixScroll(); renderDetail(); }
      break;

    case App::Menu:
      if (e == Btn::Up)   { menuSel = (menuSel + kMenuCount - 1) % kMenuCount; renderMenu(); }
      else if (e == Btn::Down) { menuSel = (menuSel + 1) % kMenuCount; renderMenu(); }
      else if (e == Btn::Select) { doMenuAction(menuSel); }
      else if (e == Btn::Back) { app = App::Results; renderResults(); }
      break;

    case App::About:
      // any button returns to the menu
      app = App::Menu;
      renderMenu();
      break;

    default:
      break;
  }
}

// ---------------------------------------------------------------------------
//  Arduino entry points
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(50);
  Serial.printf("\n=== %s v%s ===\n", FW_NAME, FW_VERSION);

  buttons.begin();
  configMgr.begin();
  display.begin();

  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

  // Hold BACK at power-on (or no config) -> captive portal.
  bool forcePortal = buttons.heldFor(Btn::Back, FORCE_PORTAL_HOLD_MS);
  if (forcePortal || !configMgr.exists()) {
    startPortal();
    return;
  }

  configMgr.load(cfg);
  db.begin();
  db.setHomeSsid(cfg.ssid);
  logger.begin();

  if (cause == ESP_SLEEP_WAKEUP_TIMER) {
    // Scheduled wake -> refresh data.
    doScan();
  } else if (store.load(nets, geoIp, geoCountry, geoCity) && !nets.empty()) {
    // Button/power wake or reset with cache -> instant display.
    Serial.printf("[main] loaded %d cached networks\n", (int)nets.size());
  } else {
    // First run after config, no cache -> scan.
    doScan();
  }

  app = App::Results;
  sel = scrollTop = 0;
  renderResults();
  lastActivity = millis();
}

void loop() {
  if (app == App::Portal) {
    portal.handle();
    if (portal.getConfigReceived()) {
      portal.getConfig(cfg);
      bool saved = configMgr.save(cfg);
      theme.showMessage(saved ? "Saved!" : "Save failed",
                        saved ? "Restarting in 3 seconds..." : "Storage error.",
                        saved ? ("Will connect to: " + cfg.ssid) : "Retrying...");
      delay(3000);
      ESP.restart();
    }
    return;
  }

  Btn e = buttons.poll();

  if (buttons.longPressPower()) {
    deepSleep();  // never returns
  }

  if (e != Btn::None) {
    lastActivity = millis();
    handleButton(e);
  }

  if (millis() - lastActivity > UI_IDLE_SLEEP_MS) {
    deepSleep();  // never returns
  }

  delay(15);
}
