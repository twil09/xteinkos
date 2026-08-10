# Vix OS

A custom, **portrait** operating system for the **Xteink X3** e-reader
(ESP32-C3, 792×528 e-ink, run at **528×792**). A book reader, a folder of
**offline games**, Wi-Fi + QR tools, reading stats, and sleep — built on the
MIT-licensed [community-sdk](https://github.com/crosspoint-reader/community-sdk)
hardware layer (the proven X3 `EInkDisplay` driver), so it runs natively on the
device.

## Home

A **book carousel** across the top (each cover shows the title, with a centred
**tick** once you finish the book), over a **2×2 section grid**:

- **Games** — folder of 12 offline games
- **Files** — splits into **Books** and **Images**
- **Wi-Fi** — status + connect / phone captive-portal setup
- **Settings** — device info, reading stats, sleep, QR codes

## Controls

The X3's two bottom paddles give four zones — **Back / Select / Left / Right** —
the primary controls, shown as an on-screen button bar. The two **side buttons
are Up / Down** (page turning in the reader, and scrolling elsewhere).

## Reader & formats

Paginated, word-wrapped reader from a **microSD card** (root or `/books` /
`/images`) **or** internal flash. Rendered now: `.txt .md .xtc .html .rtf`
(HTML/RTF tags stripped). Listed and routed to staged viewers: `.epub .pdf
.mobi .cbz .cbr` and images `.jpg .jpeg .png .bmp .gif`. Reading position is
saved per book.

## Reading stats

Tracks total reading time and pages, a **reading-speed score (1–100)** (from
average time per page), and per-book completion (the carousel tick). Shown in
**Settings → Reading**. Sharing stats with another Vix OS device over Wi-Fi is
the next step (a Share entry is present).

## Games (12)

2048, Tic-Tac-Toe, Sudoku, Minesweeper, 15 Puzzle, Lights Out, Snake, Memory
Match, Connect Four (2P), Reversi (vs CPU), Hangman, Blackjack.

## Sleep

Idle (or **Settings → Sleep now**, or the power button) shows a sleep screen —
a large **V**, "Vix OS", "Sleeping" — then deep-sleeps; the e-ink keeps its
image and the power button wakes it.

## Build & flash

Requires [PlatformIO](https://platformio.org/).

```bash
pio run                 # compile
pio run -t upload       # flash over USB
pio run -t uploadfs     # upload LittleFS (bundled sample book in data/books)
```

Prebuilt: in the CrossPoint web flasher pick **"Custom .bin"** →
[`firmware/xteinkos-x3-app.bin`](firmware/). See [`FLASHING.md`](FLASHING.md).
Add books on a microSD (`.txt`/… in root or `/books`) or in `data/books/` +
`uploadfs`.

## Layout

```
include/config.h       Portrait dims, pins, colors, button map, paths
lib/                   Vendored MIT SDK: EInkDisplay, InputManager, SDCardManager
src/
  main.cpp             Entry + sleep screen + deep-sleep
  DuetDisplay.*        Rotated GFXcanvas1 -> e-ink
  theme.h              Header bar, tiles, button bar, list rows
  App.h AppManager.*   App framework (stack, tick, idle, sleep request)
  HomeApp.*            Carousel + section grid
  GamesApp.* <game>.*  Games folder + one App per game
  FilesApp.* LibraryApp.* ReaderApp.* BookSource.* ProgressStore.* Stats.*
  WiFiApp.* WifiStore.* QrApp.* QrView.h  SettingsApp.*
```

## Status / notes

- Compiles clean; built on the proven SDK display/input drivers (portrait via a
  rotated canvas). ~19% flash, ~29% RAM.
- **Hardware-tested:** boots on the X3 (previous landscape build confirmed).
  This portrait build + new UI should be flashed and checked.
- **Staged:** EPUB/PDF/MOBI/CBZ/CBR rendering, JPEG/PNG decoding to e-ink, and
  live device-to-device stat sharing — recognised/entry-pointed now.

## Credits & license

Hardware layer © Open X4 E-Paper Contributors (**MIT**), vendored under `lib/`;
pin map/approach informed by CrossPoint. App code is the project owner's — add a
top-level LICENSE before distributing.
