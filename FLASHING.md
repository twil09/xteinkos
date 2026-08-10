# Flashing X3 OSINT OS

> **Read first.** These images are built from the confirmed X3 specs but have
> **not been tested on real hardware**. The display driver is the unproven part —
> the first boot may show a blank or upside-down screen (fixable, see
> [Troubleshooting](#troubleshooting)). The ESP32-C3 is **not permanently
> brickable**: you can always re-enter download mode and re-flash or restore a
> backup. Flashing **overwrites the stock X3 firmware** — back it up first.

The Xteink X3 has **16 MB** flash. Images here use a CrossPoint-compatible
partition table (`partitions.csv`): app0 at `0x10000` (6.25 MB), LittleFS
"spiffs" at `0xc90000` (3.375 MB). Chip: **ESP32-C3**, flash mode **DIO**.

## Prebuilt images

In [`firmware/`](firmware/) (also on the GitHub Releases page if one is cut):

| File | Contents | Flash offset |
|------|----------|-------------|
| `xteinkos-x3-app.bin` | Firmware application image | `0x10000` |
| `xteinkos-x3-bootloader.bin` | Second-stage bootloader | `0x0` |
| `xteinkos-x3-partitions.bin` | Partition table | `0x8000` |
| `xteinkos-x3-littlefs.bin` | Config/logs filesystem (optional — the app formats it on first boot if absent) | `0xc90000` |

## Option A — CrossPoint web flasher (recommended)

The CrossPoint web flasher (`crosspointreader.com/#flash-tools`) writes its own
bootloader + partition table and takes a single **application image** for the
custom option:

1. Open the flasher, connect the X3 over USB (WebSerial).
2. Select your model (**X3**) and choose the **"Custom .bin"** option.
3. Select **`firmware/xteinkos-x3-app.bin`** and flash.
4. Reset. You should see **"Waiting for config"**.

`xteinkos-x3-app.bin` is a plain ESP32-C3 application image (starts with `0xE9`,
declared size == file size), which is exactly what that flasher validates — so
it will **not** show the "declared size does not match file size" error you get
from a merged full-flash image.

> The custom option does not write the filesystem; that's fine — the app formats
> its LittleFS partition on first boot and comes up in the setup portal.

## Option B — esptool over USB (full, explicit)

Put the device in download mode (hold **BOOT**, tap **RESET**/replug, release
BOOT), then:

```bash
esptool --chip esp32c3 --baud 921600 write_flash \
  0x0      firmware/xteinkos-x3-bootloader.bin \
  0x8000   firmware/xteinkos-x3-partitions.bin \
  0x10000  firmware/xteinkos-x3-app.bin \
  0xc90000 firmware/xteinkos-x3-littlefs.bin
```

Flash app only (keeps existing bootloader/partitions):

```bash
esptool --chip esp32c3 --baud 921600 write_flash 0x10000 firmware/xteinkos-x3-app.bin
```

## Step 0 — back up the stock firmware (recommended)

```bash
esptool --chip esp32c3 --baud 921600 read_flash 0x0 0x1000000 x3-stock-backup.bin
```

Restore later with:

```bash
esptool --chip esp32c3 --baud 921600 write_flash 0x0 x3-stock-backup.bin
```

## Option C — build + flash from source (PlatformIO)

```bash
pio run                 # compile
pio run -t upload       # flash firmware over USB
pio run -t uploadfs     # flash the LittleFS image (config + optional data files)
pio device monitor      # serial log @ 115200
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
| "declared size … does not match file size" | You used a merged full-flash image. Use the single app image `xteinkos-x3-app.bin`. |
| Blank / garbled screen | Display driver likely needs a hardware tweak — see the tuning notes atop `lib/GxEPD2_X3/GxEPD2_368_X3.h` (gate reversal / border byte), rebuild, re-flash. Confirm SPI pins in `include/display_config.h`. |
| Image upside-down / offset | Flip the `y = HEIGHT - y - h` reversal in `_setPartialRamArea` and the `0x11` data-entry byte. |
| Won't flash / not detected | Enter download mode: hold **BOOT**, tap **RESET** (or replug USB), release BOOT. Try a lower `--baud` (e.g. 115200). |
| Buttons wrong / unresponsive | ADC centers vary slightly per unit — widen `ADC_TOLERANCE` in `include/config.h`, or recalibrate the values there. |
| Bricked? | It isn't — restore your backup: `esptool --chip esp32c3 write_flash 0x0 x3-stock-backup.bin`. |
