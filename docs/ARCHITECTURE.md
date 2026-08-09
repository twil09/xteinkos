# Architecture

X3 OSINT OS is a small, single-purpose ESP32-C3 firmware built on the Arduino
framework. At boot it either runs the **captive portal** (first run /
reconfigure) or enters the **interactive app** (normal operation): a button
-driven UI over the last/most-recent scan, backed by deep sleep for power.

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

`setup()` picks the path from the wake cause and cached state:

- **Timer wake** → run a full scan cycle, then show results.
- **Power/button wake or cold reset with a cache** → load `/last_scan.json`
  and show results immediately (no scan — instant).
- **First run after config, no cache** → scan, then show results.

`loop()` then runs the interactive app: it polls the buttons, dispatches to the
current screen, and deep-sleeps on inactivity, on **Sleep now**, or on a Power
long-press. Deep sleep arms two wake sources — the power button
(`esp_deep_sleep_enable_gpio_wakeup`, GPIO3 low) and the scan-interval timer —
so pressing power resumes instantly while the timer drives periodic re-scans.
In **portal** mode `loop()` instead pumps DNS/HTTP until a config is submitted,
then saves and reboots.

### UI state machine

```
        ┌──────── Back ────────┐
        ▼                       │
     RESULTS ──Select──► DETAIL │   (Up/Down = prev/next AP in DETAIL)
        │  ▲                    │
      Back │ └──── Back ────────┘
        ▼  │
      MENU ─Select→ {Rescan | Reconfigure | Toggle GeoIP | Sleep | About}
```

Each screen is a full-frame render (`DuetTheme::showResults/showDetail/
showMenu/showMessage`); the app owns the selection index and scroll offset and
re-renders on each handled button event. e-ink refreshes only on state changes,
so an idle screen costs no power.

## Modules

| Module | Responsibility |
|--------|----------------|
| `main.cpp` | Boot path, interactive app state machine, scan cycle, sleep/wake |
| `config_manager` | Mount LittleFS; load/save/clear `/config.json` (ArduinoJson) |
| `portal_server` | Soft-AP, wildcard DNS, captive-probe answers, config form |
| `input_buttons` | Decode 2 ADC ladders + power button → debounced logical events |
| `results_store` | Cache / restore the last scan (`/last_scan.json`) across sleep |
| `wifi_scanner` | `WiFi.scanNetworks()` → `ScannedAP[]`, auth-mode strings |
| `osint_db` | OUI→vendor (built-in table + optional `/oui.csv`), known-net check |
| `http_manager` | `HTTPClient` GET → string/JSON, timeouts, GeoIP helper |
| `duet_theme` | Paints e-ink screens (results/detail/menu/message) in Duet style |
| `qr_display` | Encodes a URL (ricmoo/QRCode) and draws modules |
| `display_driver` | GxEPD2 wrapper; owns the paged full-window render loop |
| `lib/GxEPD2_X3` | Custom GxEPD2 SSD1677 792×528 panel class (see below) |
| `osint_logger` | Appends CSV rows to `/scans.log` with size rotation |

Dependencies flow one way: `main` wires everything together; the UI layer
(`duet_theme`, `qr_display`) depends on `display_driver`; enrichment
(`wifi_scanner`) depends on `osint_db`. Nothing outside `display_driver` and
`display_config.h` touches GxEPD2 template types.

## Display abstraction

All panel specifics live in **`include/display_config.h`**:

- `X3_PANEL_CLASS` — the GxEPD2 panel class, set to the shipped
  **`GxEPD2_368_X3`** (in `lib/GxEPD2_X3`): a faithful 792×528 adaptation of
  GxEPD2's SSD1677 `GxEPD2_426_GDEQ0426T82`, using the panel's built-in OTP
  waveforms (no custom LUT). SPI defaults to 4 MHz mode 0, under the X3's 10 MHz
  limit. It is written from the confirmed specs but not yet hardware-validated;
  tuning notes live at the top of `GxEPD2_368_X3.h`.
- `EPD_PIN_*` — SPI + control pins (X3: SCK=8, MOSI=10, CS=21, DC=4, RST=5,
  BUSY=6; SPI mode 0, 10 MHz max).
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

- The `GxEPD2_368_X3` driver is written from confirmed specs but not yet
  validated on hardware; fast/partial refresh uses the OTP waveforms rather than
  X3-tuned LUTs (fine for full refresh, which is what the UI uses).
- "Active Scan" currently behaves like passive; BLE scanning is a future add.
- Interactive polling keeps the CPU awake (light activity) until the idle
  timeout; a light-sleep-between-polls scheme would cut active power further.
- GeoIP is device-level (your public IP). Per-AP geolocation would require an
  external WiFi-geolocation service and is intentionally not built in.
- No OTA updates yet; flashing is over USB.
