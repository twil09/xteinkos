# Vix OS

A custom, **portrait** operating system for the **Xteink X3** e-reader
(ESP32-C3, 792×528 e-ink, run at **528×792**). A book reader, a folder of
**offline games**, Wi-Fi + QR tools, reading stats, and sleep — built on the
MIT-licensed [community-sdk](https://github.com/crosspoint-reader/community-sdk)
hardware layer (the proven X3 `EInkDisplay` driver), so it runs natively on the
device.

Every screen carries a status bar: the **Vix logo**, the screen title, and — on
the right — the **clock**, **Wi-Fi** signal, and **battery** level.

## Home

A **book carousel** across the top (each cover shows the title, with a centred
**tick** once you finish the book), over four sections:

- **Games** — folder of 12 offline games
- **Books** — your library, splitting into **Books** and **Images**
- **Wi-Fi** — status, connect, phone setup, and **file transfer**
- **Settings** — device info, clock/time-zone, reading stats, sleep, QR codes

Press **Left / Right** to step through the sections (Games → Books → Wi-Fi →
Settings); the **side Up / Down** buttons scroll the book carousel; **Select**
opens the highlighted section.

## Controls

The X3's two bottom paddles give four zones — **Back / Select / Left / Right** —
the primary controls, shown as an on-screen button bar. The two **side buttons
are Up / Down** (page turning in the reader, scrolling the carousel on Home).

## Reader & formats

Paginated, word-wrapped reader from a **microSD card** (root or `/books` /
`/images`) **or** internal flash. Rendered now: `.txt .md .xtc .html .rtf` and
**`.epub`** (EPUB 2/3). Still staged: `.pdf .mobi .cbz .cbr` and images.
Reading position is saved per book.

**EPUB** is unzipped on-device (vendored `miniz` inflate) — container.xml → OPF
→ spine order — and each chapter is streamed through an XHTML tag-stripper into
one cached plain-text file, which the normal paginator renders. Re-opening the
same book is instant (cached).

**Fonts & layout:** press **Select** in a book to open the menu → **Fonts &
layout**: Sans/Serif/Mono family, four text sizes, three line-spacings, three
margin widths — applied live and remembered.

**Dictionary:** put an uncompressed StarDict (`<stem>.ifo/.idx/.dict`) in
`/dict` on the SD card, then the in-book menu gains **Look up a word** — step
through the words on the page and get a scrollable definition (StarDict `.idx`
scan; `.dict.dz` not supported).

**Sync:** the in-book menu gains **Sync progress** when a KOReader sync server
is configured (set it on the phone page) — it uploads your position and, if the
server is further along, jumps to it. Uses KOReader's partial-MD5 document id.

**Bookmarks:** the in-book menu adds/removes a bookmark at the current page and
lists them to jump back; a corner mark shows on bookmarked pages. Bookmarks are
stored as byte offsets, so they survive font/size changes.

## Wi-Fi, transfer & clock

Credentials are saved and the device **auto-reconnects** on every wake. Set up
from a phone via an open **`Vix-Setup`** captive portal (scan the on-screen QR).
Once on your home network, **Wi-Fi → Transfer files** starts a web page at
**`http://vix.local/`** (a QR shows the IP for phones that can't resolve
`.local`) where you can **upload/download/delete** books and images — books land
in the Library, images in Images, sorted by file type. That page also configures
**OPDS catalogs** and the **KOReader sync** server (no on-device keyboard). Time
is set over **NTP** when connected (adjust your zone in **Settings → Time
zone**); the RTC keeps it through naps.

**Download books (OPDS):** **Books → Download books** browses saved OPDS
catalogs (seeded with Standard Ebooks + Project Gutenberg) and downloads EPUBs
straight to the Library.

## Reading stats

Tracks total reading time and pages, a **reading-speed score (1–100)** (from
average time per page), and per-book completion (the carousel tick). Shown in
**Settings → Reading**. Sharing stats with another Vix OS device over Wi-Fi is
the next step.

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
include/config.h       Portrait dims, pins, battery/net consts, button map, paths
lib/                   Vendored MIT SDK (EInkDisplay/InputManager/SDCardManager)
                       + miniz (tinfl inflate, ROM-safe renames) for EPUB
src/
  main.cpp             Entry + Net boot-connect + sleep screen + deep-sleep
  DuetDisplay.*        Rotated GFXcanvas1 -> e-ink
  theme.h              Status bar (logo/clock/wifi/battery), tiles, rows
  Battery.h Clock.*    LiPo gauge (GPIO0) + NTP wall-clock with UTC offset
  Net.*                Wi-Fi auto-reconnect, mDNS, and file-transfer server
  App.h AppManager.*   App framework (stack, tick, idle, sleep request)
  HomeApp.*            Carousel + section grid
  GamesApp.* <game>.*  Games folder + one App per game
  Epub.*               EPUB -> cached plain text (zip + inflate + tag strip)
  ReaderApp.* ReaderSettings.* BookmarkStore.*  Reader, fonts/layout, bookmarks
  Dict.*               StarDict (.ifo/.idx/.dict) word lookup
  Opds.* OpdsStore.* OpdsApp.*  OPDS catalog browse + EPUB download
  KoSync.* KoSyncStore.*        KOReader progress sync (partial-MD5 doc id)
  FilesApp.* LibraryApp.* BookSource.* ProgressStore.* Stats.*
  WiFiApp.* WifiStore.* QrApp.* QrView.h  SettingsApp.*
```

## Status / notes

- Compiles clean; built on the proven SDK display/input drivers (portrait via a
  rotated canvas). ~20% flash, ~30% RAM.
- **Hardware-tested:** boots and flashes on the X3; portrait orientation
  confirmed. Battery gauge uses the X3 board profile (ADC GPIO0, ×2.0 divider).
- **Staged:** PDF/MOBI/CBZ/CBR rendering and JPEG/PNG decoding to e-ink
  (recognised/entry-pointed now). EPUB text renders; images/footnotes inside
  EPUBs are dropped for now.

## Credits & license

Hardware layer © Open X4 E-Paper Contributors (**MIT**), vendored under `lib/`.
The EPUB/bookmark/font/dictionary/OPDS/KOSync approaches and the `miniz` build
(with its ROM-symbol renames) are adapted from **CrossPoint Reader** © Dave
Allie (**MIT**); `miniz` © Rich Geldreich et al. (MIT/public domain). KOReader
sync protocol/document-id and the StarDict format are community standards. Pin
map informed by CrossPoint/FreeInk. App code is the project owner's — add a
top-level LICENSE before distributing.
