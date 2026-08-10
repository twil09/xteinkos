# Usage

## Controls

Two large paddles (four zones: **Back / Confirm / Left / Right**), two side
buttons (**right = Up, left = Down**), and a **Power** button.

| Screen | Up/Down/Left/Right | Confirm | Back | Power |
|--------|--------------------|---------|------|-------|
| Home | move / scroll | open | — | (idle → sleep) |
| Most games | move cursor / play | place / new game | home | — |
| Minesweeper | move | reveal | **flag** | exit |
| Blackjack | Up=Hit / Down=Stand | Hit / new hand | home | — |
| Snake | steer | restart (when over) | home | — |
| Library | select | read | home | — |
| Reader | prev / next page | — | library | — |
| Wi-Fi / QR | navigate | act | back | — |

Held Up/Down auto-repeats where it makes sense. The device deep-sleeps after
~2 min idle (not during Wi-Fi setup); press **Power** to wake — the e-ink keeps
its last image while asleep.

## Reading

Add books as `.txt` or `.md`:
- **microSD:** copy files to the card's root or a `/books` folder; they appear
  in the Library tagged `[SD]`.
- **Internal flash:** put files in `data/books/` and run `pio run -t uploadfs`.

Open Library → pick a book → arrows turn pages. Your position is saved per book.

## Wi-Fi & QR

- **Wi-Fi Setup → Set up via phone:** the device makes an open `xteinkOS-Setup`
  network; join it (scan the on-screen QR), a page opens, enter your home Wi-Fi,
  Save. The device stores it and connects. **Connect to saved** reconnects later.
- **QR Codes:** Left/Right cycles between the project link, a Wi-Fi-share code,
  and the device's address (when connected).

## e-ink behaviour

The screen only redraws on change, so it's stable and low-power while you think.
Some faint ghosting after many fast refreshes is normal and clears on the next
full refresh (e.g. re-entering a screen).

## Flashing

See [`../FLASHING.md`](../FLASHING.md): CrossPoint web flasher → "Custom .bin" →
`firmware/xteinkos-x3-app.bin`. Back up your stock firmware first.
