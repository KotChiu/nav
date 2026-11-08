# soRound_os Reference Boundary

Checked on 2026-11-08 against `soBigRice/soRound_os`.

The upstream repository does not contain a top-level `LICENSE`, `COPYING`, or
equivalent license file. Its firmware targets ESP-IDF 6 and LVGL 9, while this
board uses Arduino ESP32 3.x and LVGL 8.3. Directly copying source, bitmap
assets, fonts, or generated animation data is therefore not appropriate.

The useful reference is at the product and interaction level:

- several low-frequency watch faces;
- a calm companion character with blink and tap reactions;
- a compact stock watchlist;
- small, localized redraws instead of continuous full-screen animation.

MotoNav implements those ideas independently for the ST7701 466x466 logical
canvas:

- `WatchPage.h` and the `watch*` LVGL objects show iPhone-synced time.
- `RobotPage.h` and the `robot*` LVGL objects provide an original companion.
- `StockPage.h`, `StockBridge.swift`, and `#STK` provide the quote path.
- The iPhone fetches quotes; firmware only parses and renders a compact list.

The robot deliberately avoids continuous full-screen motion. Breathing was
removed after hardware testing because every LVGL invalidation causes a logical
frame transfer on this RGB panel. Blinking and tap reactions preserve the
character without risking the flicker seen with continuous redraws.
