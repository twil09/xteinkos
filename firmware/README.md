# Prebuilt firmware images

Ready-to-flash builds of X3 OSINT OS for the Xteink X3 (ESP32-C3, 4 MB flash,
DIO @ 80 MHz). See [`../FLASHING.md`](../FLASHING.md) for full instructions,
backup steps, and troubleshooting.

| File | Contents | Flash offset |
|------|----------|-------------|
| `xteinkos-x3-full.bin` | Complete image: bootloader + partitions + boot_app0 + firmware + seeded config FS | **`0x0`** |
| `xteinkos-x3-app.bin` | Firmware (application) only | `0x10000` |
| `xteinkos-x3-littlefs.bin` | Config filesystem only | `0x310000` |

**Web / serial flasher:** use `xteinkos-x3-full.bin` at offset `0x0`.

> ⚠️ Built from confirmed X3 specs but **not yet tested on hardware** — the
> display driver may need a small tweak on first boot (see FLASHING.md). Flashing
> overwrites the stock firmware; back it up first. The device is recoverable over
> USB, so it is not permanently brickable.

These binaries are committed here because the tooling used to build them could
not cut a GitHub Release. They are regenerated from source with `pio run`; do not
hand-edit them.
