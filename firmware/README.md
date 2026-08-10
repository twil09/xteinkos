# Prebuilt firmware images

Ready-to-flash builds of **Vix OS** for the Xteink X3 (ESP32-C3, 16 MB, DIO).
See [`../FLASHING.md`](../FLASHING.md) for full instructions and backup steps.

| File | Contents | Offset |
|------|----------|--------|
| `xteinkos-x3-app.bin` | Firmware application image | `0x10000` |
| `xteinkos-x3-bootloader.bin` | Bootloader | `0x0` |
| `xteinkos-x3-partitions.bin` | Partition table | `0x8000` |

**CrossPoint web flasher (recommended):** **"Custom .bin"** → `xteinkos-x3-app.bin`.
It's a plain ESP32-C3 app image (magic `0xE9`, declared size == file size),
exactly what that flasher expects. Regenerated from source with `pio run`; don't
hand-edit.
