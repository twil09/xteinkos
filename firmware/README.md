# Prebuilt firmware images

Ready-to-flash builds of X3 OSINT OS for the Xteink X3 (ESP32-C3, **16 MB**
flash, DIO). See [`../FLASHING.md`](../FLASHING.md) for full instructions,
backup steps, and troubleshooting.

| File | Contents | Flash offset |
|------|----------|-------------|
| `xteinkos-x3-app.bin` | Firmware application image | `0x10000` |
| `xteinkos-x3-bootloader.bin` | Second-stage bootloader | `0x0` |
| `xteinkos-x3-partitions.bin` | Partition table | `0x8000` |
| `xteinkos-x3-littlefs.bin` | Config/logs filesystem (optional) | `0xc90000` |

**CrossPoint web flasher (recommended):** choose the **"Custom .bin"** option and
select **`xteinkos-x3-app.bin`**. It's a plain ESP32-C3 app image (magic `0xE9`,
declared size == file size), which is what that flasher expects — so it avoids
the "declared size does not match file size" error a merged full image causes.

> ⚠️ Built from confirmed X3 specs but **not yet tested on hardware** — the
> display driver may need a small tweak on first boot (see FLASHING.md). Flashing
> overwrites the stock firmware; back it up first. The device is recoverable over
> USB, so it is not permanently brickable.

Regenerated from source with `pio run` (+ `pio run -t buildfs` for the FS image);
do not hand-edit.
