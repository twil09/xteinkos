# xteinkos — X3 OSINT OS

Custom firmware for the **Xteink X3** e-reader (ESP32-C3, 792×528 monochrome
e-ink). It turns the device into a passive WiFi environment scanner with a
first-run captive-portal setup, an **interactive button-driven UI**, offline
vendor/known-network enrichment, optional HTTP data enrichment, per-network QR
codes, and a Duet-styled e-ink UI — with deep-sleep power management and
button/timer wake.

> **Responsible use.** This tool performs **passive** WiFi scanning — it listens
> to the beacon frames access points already broadcast (the same data your
> phone's WiFi list shows). It does **not** deauth, inject packets, capture
> traffic, or crack anything. The captive portal only collects **your own**
> home WiFi credentials to provision **your own** device. Logging nearby
> networks (wardriving) is regulated differently around the world — know and
> follow the laws where you use it, and don't log or enrich networks you have
> no right to.

---

## Features

- **Captive setup portal** — boots an `X3-OSINT` soft-AP; the config page
  auto-opens on your phone (Duet-styled, mobile-responsive).
- **Interactive UI** — browse results, open per-network detail, and act from a
  menu, all with the X3's buttons. Deep-sleeps on inactivity; wakes on the power
  button or the scan timer, showing cached results instantly.
- **Persistent config** — stored as `/config.json` on LittleFS; survives
  reboots and deep sleep. Last scan cached to `/last_scan.json`.
- **Passive WiFi scan** — SSID, BSSID, RSSI, channel, auth mode.
- **Offline enrichment** — MAC→vendor via a built-in OUI table (extendable with
  `/oui.csv`), and a known/unknown badge from your home SSID + an optional
  `/known_networks.txt`.
- **Online enrichment (optional)** — device-level GeoIP from a data server you
  run (`GET <server>/api/geoip`).
- **Duet e-ink UI** — dark header bar, outlined result cards, known/unknown
  badges, GeoIP card, status line, and a QR code footer.
- **QR linking** — encodes your dashboard/results URL for quick phone access.
- **CSV logging** — appends every scan to `/scans.log` with size-based rotation.
- **Deep sleep** — sleeps between scans on a configurable interval for battery
  life; the e-ink image persists while asleep.

## Controls

The X3 has two large split paddles (4 zones: Back / Select / Up / Down) plus two
side buttons (right = Up, left = Down) and a power button.

| Screen | Up / Down | Select | Back | Power |
|--------|-----------|--------|------|-------|
| **Results** | move selection | open detail | menu | *(hold)* sleep |
| **Detail** | prev / next network | — | back to list | *(hold)* sleep |
| **Menu** | move | choose | back to results | *(hold)* sleep |

Menu actions: **Rescan now**, **Reconfigure WiFi** (clears config → portal),
**Toggle GeoIP mode**, **Sleep now**, **About**. Hold **Back** during power-on to
force the setup portal.

## Repository layout

```
platformio.ini            Build config (env:x3)
include/
  config.h                Constants: colors, sizes, timeouts, buttons, paths
  display_config.h        Panel class (custom X3 driver), SPI pins, fonts
lib/
  GxEPD2_X3/              Custom GxEPD2 SSD1677 792x528 driver (GPL-3.0)
src/
  main.cpp                Boot + interactive app state machine + sleep/wake
  config_manager.*        LittleFS + JSON config persistence
  portal_server.*         Soft-AP + DNS + captive web form
  input_buttons.*         ADC-ladder + power button decode (debounce/repeat)
  results_store.*         Cache last scan to LittleFS for instant wake
  wifi_scanner.*          Passive scan + enrichment glue
  osint_db.*              OUI vendor lookup + known-network check
  http_manager.*          HTTP(S) fetch + JSON + GeoIP helper
  duet_theme.*            Duet-styled e-ink UI (results/detail/menu/message)
  qr_display.*            QR generation + module rendering
  display_driver.*        GxEPD2 wrapper (paged render loop)
  osint_logger.*          CSV scan log with rotation
data/
  default_config.json     Seed config (configured=false -> portal on first boot)
web/
  portal.html             Canonical captive-portal page (embedded in firmware)
docs/
  ARCHITECTURE.md         How the pieces fit together
  USAGE.md                End-user + operator guide
```

## Build & flash

Requires [PlatformIO](https://platformio.org/) (`pip install platformio` or the
VS Code extension).

```bash
# Compile
pio run

# Flash firmware over USB
pio run -t upload

# Upload the LittleFS image (data/ -> device filesystem: default config,
# optional oui.csv / known_networks.txt)
pio run -t buildfs
pio run -t uploadfs

# Serial monitor
pio device monitor
```

> **Before your first real build, open `include/display_config.h`.** The X3
> uses an **SSD1677** controller on a 3.68" **792×528** panel. The pin map is
> pre-filled with the confirmed X3 wiring (SCK=8, MOSI=10, CS=21, DC=4, RST=5,
> BUSY=6; SPI mode 0, **10 MHz max**). GxEPD2 has no stock class for 792×528,
> so `X3_PANEL_CLASS` defaults to the stock SSD1677 4.26" class
> (`GxEPD2_426_GDEQ0426T82`) — right controller, wrong geometry (800×480). For a
> correct image you must supply a **custom 792×528 SSD1677 GxEPD2 class** (copy
> `GxEPD2_426_GDEQ0426T82`, set WIDTH=792/HEIGHT=528, clamp SPI to 10 MHz, and
> use the X3 LUT waveforms) and point `X3_PANEL_CLASS` at it. See
> `include/display_config.h` for the reference links. Everything else keys off
> the `DISPLAY_WIDTH`/`DISPLAY_HEIGHT` build flags (792×528).

## Prebuilt images / flashing

Don't want to build from source? Ready-to-flash binaries live in
[`firmware/`](firmware/), and [`FLASHING.md`](FLASHING.md) has the full guide
(CrossPoint web flasher, esptool, offsets, stock-firmware backup,
troubleshooting). Short version: in the **CrossPoint web flasher**, pick
**"Custom .bin"** and select **`firmware/xteinkos-x3-app.bin`** (a plain
ESP32-C3 app image — do *not* use a merged full-flash image, which the flasher
rejects with a "declared size" error). The X3 has **16 MB** flash. These images
are **untested on real hardware** — read the notes in `FLASHING.md` first.

## First run

1. Flash firmware + filesystem.
2. On boot the display shows **“Waiting for config”**.
3. Join the **`X3-OSINT`** WiFi (password `password123`) from your phone.
4. The setup page opens automatically — enter your home WiFi, pick a mode,
   optionally set a data server URL and scan interval, then **Save**.
5. The device reboots, connects, scans, renders results, and deep-sleeps until
   the next interval.

To reconfigure later, hold the **BOOT** button (GPIO9) during power-on to force
the portal, or delete `/config.json`.

See [`docs/USAGE.md`](docs/USAGE.md) for the full guide and the optional data
server, and [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) for internals.

## Configuration reference (`/config.json`)

| Key             | Type   | Meaning                                        |
|-----------------|--------|------------------------------------------------|
| `ssid`          | string | Home WiFi SSID to join                         |
| `password`      | string | Home WiFi password                             |
| `mode`          | string | `passive` \| `active` \| `osint`               |
| `server_url`    | string | Optional enrichment server base URL            |
| `scan_interval` | number | Seconds of deep sleep between scans (min 30)   |
| `configured`    | bool   | `false` forces the captive portal              |

## License

No license file is included yet — add one before distributing. Until then, all
rights reserved by the repository owner.
