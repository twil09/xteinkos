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

## 3. Normal operation

On each cycle the device:

1. Connects to your home WiFi (shows *Connecting…*; on failure it shows an error
   and retries after 30 s).
2. Runs a passive WiFi scan and enriches each result (vendor, known/unknown).
3. If in **OSINT Mode** with a server URL, fetches device GeoIP.
4. Appends results to `/scans.log`.
5. Renders the Duet frame: dark header with network count, a GeoIP card (if
   available), result cards (strongest first) with known/unknown badges, a
   status line, and a QR code.
6. Deep-sleeps until the next interval. The e-ink image stays on screen while
   asleep.

## 4. Reconfiguring

- **Force the portal:** hold the **BOOT** button (GPIO9) while powering on.
- **Wipe config:** delete `/config.json` from LittleFS (e.g. re-run `uploadfs`
  with a fresh `default_config.json`, or add a serial command).

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
| Blank / garbled screen | Wrong panel class or pins in `display_config.h` |
| Portal never appears | Rejoin `X3-OSINT`; browse to `http://192.168.4.1` |
| Stuck “Connecting…” then retry | Wrong WiFi password, or 5 GHz-only SSID (C3 is 2.4 GHz) |
| GeoIP card missing | Not in OSINT Mode, no server URL, or server unreachable |
| Config won't stick | LittleFS mount/write failure — check serial log |
