# Usage

## Controls

Two large paddles (four zones: **Back / Confirm / Left / Right**), two side
buttons (**right = Up, left = Down**), and a **Power** button.

Every screen shows a status bar: the Vix logo + title on the left, and the
**clock, Wi-Fi signal, and battery %** on the right.

| Screen | Left/Right | Up/Down (side) | Confirm | Back |
|--------|-----------|----------------|---------|------|
| Home | **step through sections** (Games→Books→Wi-Fi→Settings) | scroll the book carousel | open section | — |
| Most games | move cursor / play | move cursor | place / new game | home |
| Minesweeper | move | move | reveal (**Back = flag**) | exit |
| Blackjack | — | Up=Hit / Down=Stand | Hit / new hand | home |
| Snake | steer | steer | restart (when over) | home |
| Library | select | select | read | home |
| Reader | prev / next page | prev / next page | — | library |
| Settings | (Time-zone row: −/+ hour) | move selection | act | home |
| Wi-Fi / QR | navigate | navigate | act | back |

The device deep-sleeps after ~2 min idle (not during Wi-Fi setup or file
transfer); press **Power** to wake — the e-ink keeps its last image while asleep.

## Reading

Add books as `.txt` or `.md`:
- **microSD:** copy files to the card's root or a `/books` folder; they appear
  in the Library tagged `[SD]`.
- **Internal flash:** put files in `data/books/` and run `pio run -t uploadfs`.

Open Library → pick a book → arrows turn pages. Your position is saved per book.

## Wi-Fi, file transfer & clock

- **Set up via phone:** the device makes an open `Vix-Setup` network; join it
  (scan the on-screen QR), a page opens, enter your home Wi-Fi, Save. The device
  stores it and **auto-reconnects on every wake**.
- **Transfer files:** once connected, **Wi-Fi → Transfer files** shows a URL
  (`http://vix.local/`) and a QR (encoding the IP, so it works even where
  `.local` doesn't). Open it on a phone on the **same network** to
  upload/download/delete books and images — books go to the Library, images to
  Images, sorted automatically by file type. Uploads land in internal flash
  (~3.5 MB); use a microSD for a large library.
- **Clock:** time is fetched over NTP when Wi-Fi connects. Set your offset in
  **Settings → Time zone** (Left/Right changes the hour). The RTC keeps time
  through sleep; a full power-off re-syncs on the next connect.
- **QR Codes:** Left/Right cycles between the project link, a Wi-Fi-share code,
  and the device's address (when connected).

## e-ink behaviour

The screen only redraws on change, so it's stable and low-power while you think.
Some faint ghosting after many fast refreshes is normal and clears on the next
full refresh (e.g. re-entering a screen).

## Flashing

See [`../FLASHING.md`](../FLASHING.md): CrossPoint web flasher → "Custom .bin" →
`firmware/xteinkos-x3-app.bin`. Back up your stock firmware first.
