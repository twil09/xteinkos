# xteinkos — X3 OSINT OS

Custom firmware for the **Xteink X3** e-reader (ESP32-C3, 792×528 monochrome
e-ink). It turns the device into a passive WiFi environment scanner with a
first-run captive-portal setup, offline vendor/known-network enrichment,
optional HTTP data enrichment, a QR code linking to your results, and a
Duet-styled e-ink UI.

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
- **Persistent config** — stored as `/config.json` on LittleFS; survives
  reboots and deep sleep.
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

## Repository layout

```
platformio.ini            Build config (env:x3)
include/
  config.h                Constants: colors, sizes, timeouts, paths, modes
  display_config.h        >>> Panel class, SPI pins, fonts — EDIT FOR YOUR HW <<<
src/
  main.cpp                Boot + orchestration
  config_manager.*        LittleFS + JSON config persistence
  portal_server.*         Soft-AP + DNS + captive web form
  wifi_scanner.*          Passive scan + enrichment glue
  osint_db.*              OUI vendor lookup + known-network check
  http_manager.*          HTTP(S) fetch + JSON + GeoIP helper
  duet_theme.*            Duet-styled e-ink rendering
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

> **Before your first real build, open `include/display_config.h`.** The X3's
> exact e-ink controller isn't published, so the firmware defaults to a common
> GxEPD2 panel class (`GxEPD2_750_T7`, 800×480) and a placeholder SPI pin map so
> it compiles out of the box. Set `X3_PANEL_CLASS` to the class that matches
> your controller and correct the `EPD_PIN_*` pins for your wiring, or the
> screen will stay blank / garbled. Everything else keys off the
> `DISPLAY_WIDTH`/`DISPLAY_HEIGHT` build flags (792×528).

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
