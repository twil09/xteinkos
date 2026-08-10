# Flashing xteinkOS

> **Read first.** Built on the community-sdk's proven X3 `EInkDisplay` driver, so
> it should boot and render — but it hasn't been verified end-to-end on physical
> hardware. The ESP32-C3 is **not permanently brickable**: you can always
> re-enter download mode and re-flash or restore a backup. Flashing **overwrites
> the stock X3 firmware** — back it up first.

The Xteink X3 has **16 MB** flash. Images use a CrossPoint-compatible partition
table (`partitions.csv`): app0 at `0x10000`, LittleFS "spiffs" at `0xc90000`.
Chip **ESP32-C3**, flash mode **DIO**.

## Prebuilt images (in `firmware/`)

| File | Contents | Offset |
|------|----------|--------|
| `xteinkos-x3-app.bin` | Firmware application image | `0x10000` |
| `xteinkos-x3-bootloader.bin` | Bootloader | `0x0` |
| `xteinkos-x3-partitions.bin` | Partition table | `0x8000` |

## Option A — CrossPoint web flasher (recommended)

`crosspointreader.com/#flash-tools` writes its own bootloader + partition table
and takes a single **application image**:

1. Connect the X3 over USB (WebSerial).
2. Select model **X3** → **"Custom .bin"**.
3. Choose **`xteinkos-x3-app.bin`** and flash.
4. Reset → the xteinkOS home launcher appears.

This is a plain ESP32-C3 app image (magic `0xE9`, declared size == file size —
verified), so it won't trigger the flasher's "declared size does not match file
size" error that a merged full-flash image causes.

## Option B — esptool over USB

Download mode: hold **BOOT**, tap **RESET**/replug, release BOOT.

```bash
esptool --chip esp32c3 --baud 921600 write_flash \
  0x0     firmware/xteinkos-x3-bootloader.bin \
  0x8000  firmware/xteinkos-x3-partitions.bin \
  0x10000 firmware/xteinkos-x3-app.bin
```

Back up stock first: `esptool --chip esp32c3 read_flash 0x0 0x1000000 x3-stock-backup.bin`

## First boot

Home launcher (games, Library, Wi-Fi, QR). Up/Down select, Confirm opens, Back
returns. For books: microSD (`.txt`/`.md` in root or `/books`), or
`data/books/` + `pio run -t uploadfs`.

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| "declared size … does not match file size" | Use `xteinkos-x3-app.bin`, not a merged image. |
| Blank screen | Full flash via Option B; confirm SD partition layout intact. |
| Buttons wrong | ADC ranges live in the SDK `InputManager`; can vary per unit. |
| Won't flash | Download mode (BOOT+RESET); try `--baud 115200`. |
| Want stock back | `esptool --chip esp32c3 write_flash 0x0 x3-stock-backup.bin`. |
