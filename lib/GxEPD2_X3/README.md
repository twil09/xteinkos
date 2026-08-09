# GxEPD2_368_X3 — Xteink X3 (SSD1677, 792×528) driver

`GxEPD2_368_X3` is a GxEPD2 panel class for the Xteink X3's 3.68" 792×528
SSD1677 e-paper display. GxEPD2 ships no stock class for this geometry, so this
is a **faithful adaptation of GxEPD2's `GxEPD2_426_GDEQ0426T82`** (also SSD1677,
800×480) with the resolution changed to 792×528. Every command/RAM/refresh
sequence is geometry-driven and uses the panel's built-in OTP waveforms, so no
custom LUT upload is needed for a correct full refresh.

Use it via the normal GxEPD2 template:

```cpp
#include <GxEPD2_BW.h>
#include <GxEPD2_368_X3.h>
GxEPD2_BW<GxEPD2_368_X3, GxEPD2_368_X3::HEIGHT> display(
    GxEPD2_368_X3(/*CS=*/21, /*DC=*/4, /*RST=*/5, /*BUSY=*/6));
```

## Status

Written from the confirmed X3 hardware specs (SSD1677, 792×528, pins SCK=8,
MOSI=10, CS=21, DC=4, RST=5, BUSY=6; SPI mode 0, ≤10 MHz — GxEPD2 defaults to
4 MHz) but **not yet validated on physical hardware**. Tuning notes for common
issues (upside-down image, ghosting) are at the top of `GxEPD2_368_X3.h`.

## License

Derived from **GxEPD2** by Jean-Marc Zingg (https://github.com/ZinggJM/GxEPD2),
which is licensed **GPL-3.0-or-later**. This derivative file therefore inherits
**GPL-3.0-or-later**, and any distributed firmware that links GxEPD2 is subject
to the GPL. See the top-level project notes on licensing.
