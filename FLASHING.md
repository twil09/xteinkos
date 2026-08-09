# Flashing X3 OSINT OS

> **Read first.** These images are built from the confirmed X3 specs but have
> **not been tested on real hardware**. The display driver is the unproven part —
> the first boot may show a blank or upside-down screen (fixable, see
> [Troubleshooting](#troubleshooting)). The ESP32-C3 is **not permanently
> brickable**: you can always re-enter download mode and re-flash or restore a
> backup. Flashing **overwrites the stock X3 firmware and bootloader** — back it
> up first.

## Prebuilt images

Build artifacts are not committed to git. Get them from the
[GitHub Releases](../../releases) page, or build them yourself (below).

| File | Contents | Flash offset |
|------|----------|-------------|
| `xteinkos-x3-full.bin` | Everything: bootloader + partition table + boot_app0 + firmware + seeded config filesystem | **`0x0`** |
| `xteinkos-x3-app.bin` | Firmware (application) only | `0x10000` |
| `xteinkos-x3-littlefs.bin` | Config filesystem only (seeds the first-boot portal) | `0x310000` |

Image assumptions: **4 MB flash, DIO mode, 80 MHz**. If your unit differs,
rebuild (see below) or adjust the flash args.

## Step 0 — back up the stock firmware (recommended)

Put the device in download mode (hold **BOOT**, tap **RESET**/replug, release
BOOT) and read the whole flash so you can always restore it:

```bash
esptool --chip esp32c3 --baud 921600 read_flash 0x0 0x400000 x3-stock-backup.bin
```

Restore later with:

```bash
esptool --chip esp32c3 --baud 921600 write_flash 0x0 x3-stock-backup.bin
```

## Option A — web flasher (easiest)

Most ESP web flashers (ESP Web Tools / esptool-js based, incl. community X3
flashers) take a **single merged image written at `0x0`**:

1. Connect the X3 over USB; put it in download mode if the flasher asks.
2. Select **`xteinkos-x3-full.bin`** and set the offset to **`0x0`**
   (single-file flashers assume `0x0` automatically).
3. Flash, then reset. You should see **"Waiting for config"**.

If the flasher instead shows separate slots/offsets, use the three files at the
offsets in the table above. If it only writes an application image, use
`xteinkos-x3-app.bin` at `0x10000` (this keeps whatever bootloader/partition
table is already on the device — only works if it matches this layout).

## Option B — esptool over USB

Full image in one shot:

```bash
esptool --chip esp32c3 --baud 921600 write_flash 0x0 xteinkos-x3-full.bin
```

Or the parts individually:

```bash
esptool --chip esp32c3 --baud 921600 write_flash \
  0x10000  xteinkos-x3-app.bin \
  0x310000 xteinkos-x3-littlefs.bin
# (bootloader.bin @ 0x0 and partitions.bin @ 0x8000 if replacing those too)
```

## Option C — build + flash from source (PlatformIO)

```bash
pio run                 # compile
pio run -t upload       # flash firmware over USB
pio run -t uploadfs     # flash the LittleFS image (config + optional data files)
pio device monitor      # serial log @ 115200
```

To regenerate the merged image yourself:

```bash
BOOT_APP0=$(python -c "import os,glob;print(glob.glob(os.path.expanduser('~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin'))[0])")
python -m esptool --chip esp32c3 merge-bin -o xteinkos-x3-full.bin \
  --flash-mode dio --flash-freq 80m --flash-size 4MB \
  0x0 .pio/build/x3/bootloader.bin \
  0x8000 .pio/build/x3/partitions.bin \
  0xe000 "$BOOT_APP0" \
  0x10000 .pio/build/x3/firmware.bin \
  0x310000 .pio/build/x3/littlefs.bin
```

## First boot

1. Screen shows **"Waiting for config"** with the AP details.
2. Join WiFi **`X3-OSINT`** (password `password123`) from a phone.
3. The setup page opens automatically (or browse to `http://192.168.4.1`).
4. Enter home WiFi + options, **Save**. The device reboots, scans, and drops
   into the interactive results UI.

See [`docs/USAGE.md`](docs/USAGE.md) for controls and day-to-day use.

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| Blank / garbled screen | Most likely the display driver needs a hardware tweak — see the tuning notes atop `lib/GxEPD2_X3/GxEPD2_368_X3.h` (gate reversal / border byte), rebuild, re-flash. Confirm SPI pins in `include/display_config.h`. |
| Image upside-down / offset | Flip the `y = HEIGHT - y - h` reversal in `_setPartialRamArea` and the `0x11` data-entry byte. |
| Won't flash / not detected | Enter download mode: hold **BOOT**, tap **RESET** (or replug USB), release BOOT. Try a lower `--baud` (e.g. 115200). |
| Buttons wrong / unresponsive | ADC centers vary slightly per unit — widen `ADC_TOLERANCE` in `include/config.h`, or recalibrate the values there. |
| Wrong flash size error | Your unit may not be 4 MB — rebuild with the right `board_upload.flash_size` / partition table. |
| Bricked? | It isn't — restore your backup: `esptool --chip esp32c3 write_flash 0x0 x3-stock-backup.bin`. |
