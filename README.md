# xteinkOS

A custom, **Duet-styled** operating system for the **Xteink X3** e-reader
(ESP32-C3, 792×528 e-ink): a home launcher for a set of **offline games**, a
**book reader**, and **Wi-Fi + QR** tools. Built on the MIT-licensed
[community-sdk](https://github.com/crosspoint-reader/community-sdk) hardware
layer (the same proven `EInkDisplay` driver CrossPoint uses), so it runs
natively on the X3's screen.

> Pivoted from an earlier OSINT scanner build; see git history for that.

## Features

**Games (12):** 2048, Tic-Tac-Toe (vs unbeatable CPU), Sudoku, Minesweeper,
15 Puzzle, Lights Out, Snake, Memory Match, Connect Four (2-player), Reversi
(vs CPU), Hangman, Blackjack.

**Library:** paginated `.txt` / `.md` reader from a **microSD card** (root or
`/books`, tagged `[SD]`) **or** internal flash (LittleFS `/books`), word-wrapped,
with per-book reading position saved.

**Wi-Fi Setup:** connect to a saved network, or set one from your phone via a
captive portal (`xteinkOS-Setup` AP → form → saved to flash).

**QR Codes:** scannable codes to share your Wi-Fi, open the device's address, or
the project link.

**System:** Duet-styled scrolling launcher; idle **deep-sleep** with power-button
wake (the e-ink image persists while asleep).

Footprint: ~19% of the 16 MB flash, ~29% RAM — plenty of room to grow.

## Controls

Two large paddles split into four zones (**Back / Confirm / Left / Right**), two
side buttons (**right = Up, left = Down**), and a **Power** button.

| Screen | Up/Down/Left/Right | Confirm | Back |
|--------|--------------------|---------|------|
| Home | move / scroll | open app | — |
| Games | move / play | place / new game | home |
| Library | select | read | home |
| Reader | prev / next page | — | library |

Minesweeper uses **Back = flag**, **Power = exit**. Blackjack uses
**Up/Confirm = Hit, Down = Stand**.

## Build & flash

Requires [PlatformIO](https://platformio.org/).

```bash
pio run                 # compile
pio run -t upload       # flash over USB
pio run -t uploadfs     # upload LittleFS (bundled sample book in data/books)
pio device monitor      # serial @ 115200
```

**Prebuilt image:** in the CrossPoint web flasher pick **"Custom .bin"** and
select [`firmware/xteinkos-x3-app.bin`](firmware/). See [`FLASHING.md`](FLASHING.md)
(and back up your stock firmware first).

Books: drop `.txt`/`.md` on a microSD (root or `/books`), or add them to
`data/books/` and run `pio run -t uploadfs`.

## Layout

```
platformio.ini         Build config (X3, 16 MB, CrossPoint partitions)
partitions.csv         16 MB partition table
include/config.h       Screen size, pins, colors, button map, paths
lib/                   Vendored MIT SDK: EInkDisplay, InputManager, SDCardManager
src/
  main.cpp             Entry point + deep-sleep
  DuetDisplay.*        GFXcanvas1 -> e-ink bridge
  theme.h              Duet drawing helpers
  Buttons.h            Logical button facade
  App.h / AppManager.* App framework (stack + redraw/tick loop)
  HomeApp.*            Launcher
  <game>.*             one App per game
  LibraryApp.* ReaderApp.* BookSource.* ProgressStore.*   reader
  WiFiApp.* WifiStore.* QrApp.* QrView.h                   wi-fi + QR
```

## Status / notes

- Compiles clean and is built on the proven SDK display/input drivers.
- **Not yet verified on physical hardware** (no device available in the build
  environment). SD and Wi-Fi build cleanly but are untested on the shared SPI
  bus; internal-flash reading is the guaranteed fallback.
- Next: EPUB, Wi-Fi book upload + NTP clock, BQ27220 battery gauge, more games.

## Credits & license

Hardware layer: **community-sdk** (`EInkDisplay`, `InputManager`,
`SDCardManager`) © Open X4 E-Paper Contributors, **MIT** — vendored under `lib/`.
Display pin map/approach informed by the CrossPoint reader project. Application
code here is the project owner's; add a top-level LICENSE before distributing.
