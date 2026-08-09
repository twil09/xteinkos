# Usage guide

## 1. Build & flash

```bash
pio run                 # compile
pio run -t upload       # flash firmware over USB
pio run -t buildfs      # build the LittleFS image from data/
pio run -t uploadfs     # upload it (seeds default config + optional data files)
pio device monitor      # watch serial logs @115200
```

If you see a blank or garbled screen, the panel class/pins are wrong for your
hardware — edit `include/display_config.h` (see the README note) and re-flash.

## 2. First-run setup (captive portal)

1. Power on. The screen shows **“Waiting for config”** with the AP details.
2. On your phone, join WiFi **`X3-OSINT`** (password `password123`).
3. Most phones pop the setup page automatically. If not, open a browser to
   `http://192.168.4.1`.
4. Fill in the form:
   - **WiFi Network / Password** — your home WiFi (what the device joins to run).
   - **Reconnaissance Mode**
     - *Passive Scan* — WiFi scan only, no network calls.
     - *OSINT Mode* — WiFi scan **+** HTTP enrichment (GeoIP). Needs internet.
     - *Active Scan* — same as passive today (BLE reserved for later).
   - **Data Server URL** *(optional)* — base URL of your enrichment server,
     e.g. `http://192.168.1.100:5000`.
   - **Scan Interval** — minutes between scans (deep-sleep time).
5. Tap **Save & Start Scanning**. The device stores the config and reboots.

## 3. Normal operation & controls

After setup the device scans, then drops into an **interactive results screen**
you drive with the buttons.

### Buttons

The X3 has two large split paddles (four zones — Back / Select / Up / Down),
two side buttons (right = Up, left = Down), and a power button.

| Screen | Up / Down | Select | Back | Power (hold) |
|--------|-----------|--------|------|--------------|
| **Results** | move selection | open **Detail** | open **Menu** | sleep |
| **Detail** | previous / next network | — | back to list | sleep |
| **Menu** | move | choose item | back to results | sleep |

Held Up/Down auto-repeats for fast scrolling. A short press of Power does
nothing (so you can't sleep by accident); hold it (~0.8 s) to sleep.

### Screens

- **Results** — dark header with the network count and (if available) GeoIP;
  one row per network, strongest first, each showing SSID, signal, vendor,
  security and a `[Known]`/`[Unknown]` tag. The selected row is highlighted.
- **Detail** — full info for one network (BSSID, vendor, signal, channel,
  security, known status, geo) plus a QR code. With a server URL the QR opens
  the dashboard for that AP; otherwise it's a standard WiFi QR of the SSID.
- **Menu** — **Rescan now**, **Reconfigure WiFi**, **Toggle GeoIP mode**,
  **Sleep now**, **About**.

### Sleep & wake

- After **60 s** of no input (or **Menu → Sleep now**, or holding Power) the
  device deep-sleeps. The e-ink image stays on screen while asleep.
- It wakes on the **power button** (shows the cached results instantly, no
  re-scan) or on the **scan-interval timer** (re-scans, then shows results).

Each scan still connects to WiFi, enriches (vendor / known / optional GeoIP),
appends to `/scans.log`, and caches the results to `/last_scan.json`.

## 4. Reconfiguring

- **Force the portal:** hold **Back** while powering on (~1.2 s).
- **From the device:** Menu → **Reconfigure WiFi** (clears config and reboots
  into the portal).
- **Wipe config manually:** delete `/config.json` from LittleFS (e.g. re-run
  `uploadfs` with a fresh `default_config.json`).

## 5. Optional data files (LittleFS)

Drop these into `data/` before `pio run -t buildfs && pio run -t uploadfs`:

- **`known_networks.txt`** — one SSID or BSSID per line; matches show a *Known*
  badge. `#` comments allowed.
  ```
  # my networks
  HomeWiFi
  A1:B2:C3:D4:E5:F6
  ```
- **`oui.csv`** — extended MAC-vendor table used as a fallback beyond the
  built-in list. One entry per line: `AABBCC,Vendor Name` (prefix may include
  `:` or `-`).
  ```
  001A2B,Example Networks
  B8:27:EB,Raspberry Pi
  ```

## 6. Optional enrichment server

OSINT Mode fetches GeoIP from `GET <server_url>/api/geoip`. Any server that
returns JSON with `country`/`city` (and optionally `ip`) works. Minimal Flask
example:

```python
from flask import Flask, request, jsonify
app = Flask(__name__)

@app.get("/api/geoip")
def geoip():
    ip = request.args.get("ip") or request.headers.get(
        "X-Forwarded-For", request.remote_addr)
    # look ip up in your own GeoIP database here...
    return jsonify(ip=ip, country="United States", city="Seattle")

app.run(host="0.0.0.0", port=5000)
```

Point **Data Server URL** at `http://<host>:5000`. Enrichment failures are
non-fatal — the scan still renders without GeoIP.

## 7. Reading the log

`/scans.log` is CSV with a header:

```
# type,uptime_ms,ssid,bssid,rssi_dbm,channel,auth,vendor,known,country,city
wifi,10432,HomeWiFi,A1:B2:C3:D4:E5:F6,-52,6,WPA2,TP-Link,yes,,
```

Pull it off the device with any LittleFS tool (e.g. `pio run -t` filesystem
download utilities, or add a serial/HTTP dump command). It rotates to
`/scans.log.1` once it passes 256 KB.

## Troubleshooting

| Symptom | Likely cause / fix |
|---------|--------------------|
| Blank screen | Verify SPI pins in `display_config.h`; the driver is untested on hw |
| Image upside-down / shifted | Flip gate reversal in `lib/GxEPD2_X3/GxEPD2_368_X3.cpp` `_setPartialRamArea` (see the header's tuning notes) |
| Buttons do nothing / wrong action | Re-check ADC centers in `config.h` against your unit (values vary slightly); widen `ADC_TOLERANCE` |
| Won't wake from sleep | Power button must idle HIGH; confirm `BTN_POWER_PIN` (GPIO3) |
| Portal never appears | Rejoin `X3-OSINT`; browse to `http://192.168.4.1` |
| “WiFi failed” then results | Wrong WiFi password, or 5 GHz-only SSID (C3 is 2.4 GHz) |
| GeoIP missing | Not in OSINT Mode, no server URL, or server unreachable |
| Config won't stick | LittleFS mount/write failure — check serial log |
