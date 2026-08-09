# Architecture

X3 OSINT OS is a small, single-purpose ESP32-C3 firmware built on the Arduino
framework. It has two runtime modes selected at boot — **captive portal** (first
run / reconfigure) and **scan** (normal operation) — and leans on deep sleep to
stay power-efficient.

## Boot & mode selection

```
power on / timer wake
        │
        ▼
 mount LittleFS  ──► init e-ink display
        │
        ▼
 BOOT button held?  OR  no valid /config.json?
        │ yes                         │ no
        ▼                             ▼
  CAPTIVE PORTAL                 SCAN CYCLE
  ├ soft-AP "X3-OSINT"          ├ show "Connecting…"
  ├ DNS wildcard → 192.168.4.1  ├ join home WiFi (timeout → sleep+retry)
  ├ serve Duet config form      ├ passive WiFi scan
  ├ on submit → save config     ├ offline enrich (vendor, known)
  └ ESP.restart()               ├ optional GeoIP via HTTP
                                ├ log to /scans.log
                                ├ render Duet frame + QR
                                └ deep sleep (scan_interval)
```

In **scan** mode all work runs inside `setup()` and the device calls
`esp_deep_sleep_start()` at the end, so `loop()` never executes. On the timer
wake `setup()` runs again from the top — config is reloaded from flash, and the
cycle repeats. In **portal** mode `setup()` starts the AP and returns; `loop()`
pumps DNS/HTTP until a config is submitted, then saves and reboots.

## Modules

| Module | Responsibility |
|--------|----------------|
| `main.cpp` | Mode selection, scan-cycle orchestration, deep sleep |
| `config_manager` | Mount LittleFS; load/save/clear `/config.json` (ArduinoJson) |
| `portal_server` | Soft-AP, wildcard DNS, captive-probe answers, config form |
| `wifi_scanner` | `WiFi.scanNetworks()` → `ScannedAP[]`, auth-mode strings |
| `osint_db` | OUI→vendor (built-in table + optional `/oui.csv`), known-net check |
| `http_manager` | `HTTPClient` GET → string/JSON, timeouts, GeoIP helper |
| `duet_theme` | Composes and paints the e-ink frame in the Duet style |
| `qr_display` | Encodes a URL (ricmoo/QRCode) and draws modules |
| `display_driver` | GxEPD2 wrapper; owns the paged full-window render loop |
| `osint_logger` | Appends CSV rows to `/scans.log` with size rotation |

Dependencies flow one way: `main` wires everything together; the UI layer
(`duet_theme`, `qr_display`) depends on `display_driver`; enrichment
(`wifi_scanner`) depends on `osint_db`. Nothing outside `display_driver` and
`display_config.h` touches GxEPD2 template types.

## Display abstraction

The one hardware unknown on the X3 is the e-ink controller, so all panel
specifics live in **`include/display_config.h`**:

- `X3_PANEL_CLASS` — the GxEPD2 panel class (defaults to `GxEPD2_750_T7`).
- `EPD_PIN_*` — SPI + control pins.
- Font macros mapping the spec's size tiers to Adafruit GFX FreeFonts.

`DisplayDriver` exposes `render(paint)`, which runs GxEPD2's
`setFullWindow()/firstPage()/nextPage()` loop and calls the supplied paint
callback once per page. Because the callback may run multiple times, the UI is
built as **state + a repeatable paint pass**: `DuetTheme` accumulates the frame
(`setHeader`, `addWiFi`, `setGeoIP`, `setStatus`, `setQR`) and then `commit()`
paints it. QR encoding happens once in `setQR` (outside the loop); only cheap
module drawing runs per page.

## Color on a monochrome panel

The Duet palette is defined in real color for the HTML portal, but the X3 panel
is black/white. On-device the palette collapses to ink-on-white:

- **Header** — solid black bar, reversed (white) text.
- **Cards** — white fill, black outline.
- **Badges** — *Known* = solid black w/ white text; *Unknown* = white w/ black
  outline. Fill (not hue) carries the distinction so it reads on e-ink.

## Data & enrichment

- **Offline (always):** vendor from the OUI table; known/unknown from the home
  SSID plus optional `/known_networks.txt`.
- **Online (OSINT mode + `server_url` set):** `GET <server>/api/geoip` returns
  the device's public-IP GeoIP (`ip`/`country`/`city`, with a couple of accepted
  field spellings). Failures are non-fatal — the frame still renders.

The QR footer encodes the `server_url` (so a phone can open your dashboard) or
the project URL when no server is configured.

## Persistence & power

- Config and logs live on the **LittleFS** partition (chosen over the deprecated
  SPIFFS; same API, more robust). `board_build.filesystem = littlefs` in
  `platformio.ini`.
- Deep sleep uses the RTC timer (`esp_sleep_enable_timer_wakeup`). RAM is lost
  across sleep, but config is in flash and the e-ink image is retained, so the
  screen keeps showing the last scan while asleep.

## Known limitations / future work

- Default panel class is a placeholder; set the correct one for a real image.
- "Active Scan" currently behaves like passive; BLE scanning is a future add.
- GeoIP is device-level (your public IP). Per-AP geolocation would require an
  external WiFi-geolocation service and is intentionally not built in.
- No OTA updates yet; flashing is over USB.
