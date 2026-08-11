# Architecture

Vix OS is a small ESP32-C3 firmware: a Duet-styled **app launcher** over a
stack of **Apps** (games, reader, utilities), drawn to the X3 e-ink through the
MIT community-sdk's proven display driver.

## Layers

```
   HomeApp   <games>   LibraryApp/ReaderApp   WiFiApp   QrApp     (each: App)
       └──────────────────────┬──────────────────────────┘
                        AppManager  (stack; redraw-on-change; tick; idle-sleep)
                        │                                   │
                 DuetDisplay                             Buttons
              GFXcanvas1 (1bpp)                       logical Btn map
                        │                                   │
                EInkDisplay (SDK)                    InputManager (SDK)
                SSD1677 792×528                      ADC ladders + power
```

## Rendering

`DuetDisplay` owns an Adafruit `GFXcanvas1` (1bpp, 792×528). Its buffer layout —
99 bytes/row, MSB first, `1 = white / 0 = black` — matches
`EInkDisplay::setFramebuffer` exactly, so frames copy across with no conversion.
`present(mode)` pushes it (`FULL` for clean screens, `HALF`/`FAST` for updates).
`theme.h` is the Duet look: black header bar with reversed text, outlined rows,
inverted (filled) selection.

## App framework

`App` = `onEnter` / `onButton(Btn)` / `render(DuetDisplay&)` plus optional
`onTick()`+`tickIntervalMs()` (animation, e.g. Snake, and Wi-Fi portal service)
and `keepAwake()` (blocks idle sleep during Wi-Fi setup). `AppManager` holds a
stack: `push` enters a screen (forces one FULL refresh); an unconsumed
**Back/Power** pops; the e-ink redraws only on change or a tick that reports one.
Adding a feature = one `App` subclass + a `HomeApp` menu entry.

## Storage & reading

- `BookSource` reads a book from **SD** (`FsFile`) or **LittleFS** (`File`)
  uniformly (`size`/`seek`/`read`). `ReaderApp` streams pages (word-wrap,
  computes the next page's byte offset), keying `ProgressStore` per source.
- `LibraryApp` lists `.txt`/`.md` from SD (root + `/books`) and flash `/books`.

## Wi-Fi, transfer & clock

- `Net` owns the STA lifecycle: on boot it restores `WifiStore` creds and
  connects with `setAutoReconnect(true)`, so the link returns after every wake.
  `Net::poll()` (from the main loop) starts mDNS (`vix.local`) and kicks NTP the
  moment the link comes up, and services the **file-transfer web server** — a
  `WebServer` with upload/download/delete routes writing to LittleFS `/books`
  and `/images` (routed by file type).
- `WiFiApp` drives the UI: a soft-AP captive portal (`WebServer` + `DNSServer`)
  to capture credentials, a connect flow, and a **Transfer** screen showing the
  URL + a QR (encoding the IP for universal reach). `QrView` renders QR codes;
  `QrApp` cycles project / Wi-Fi-share / device-IP codes.
- `Clock` keeps the RTC in UTC (NTP) and applies a persisted user offset when
  formatting; `Battery` reads the X3 LiPo gauge (ADC GPIO0, ×2.0) via the
  community-sdk polynomial. Both feed the `theme.h` status bar.

## Power

`main` deep-sleeps after `IDLE_SLEEP_MS` of no activity (unless the top app is
`keepAwake`), waking on the power button (`esp_deep_sleep_enable_gpio_wakeup`).
The e-ink keeps its image while asleep.

## Hardware (config.h + vendored SDK)

SSD1677 792×528; SPI SCLK=8, MOSI=10, CS=21, DC=4, RST=5, BUSY=6; SD shares the
bus (CS=12, MISO=7 — re-enabled after the e-ink init). 16 MB flash, CrossPoint
partition layout.
