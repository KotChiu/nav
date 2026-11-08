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

- The Home page combines the clock and status surface in one place. The
  `watch*` LVGL objects use an ORBIT / weather layout with four thin ring
  segments, a red marker, large time, temperature and a forecast row. Time is
  synced from iPhone; weather comes from the existing MotoNav packet. That
  packet does not include humidity or daily highs/lows when it is in hourly
  mode, so the lower row uses the next hour instead. Large digits use Montserrat
  Black from Google Fonts under the SIL Open Font License (see
  `licenses/OFL-Montserrat.txt`).
- `RobotPage.h` and the `robot*` LVGL objects provide an independently drawn
  white, rounded companion with dark eyes, blink and tap reactions. No
  soRound or GrokBot code or artwork is embedded.
- `StockPage.h`, `StockBridge.swift`, `#STK` and `#SK` provide the quote path.
  The iPhone fetches quotes and 12-day OHLC samples, renders Chinese names to
  `TextImage` slots 13-20, and firmware draws a compact K-line per stock.

The robot deliberately avoids continuous full-screen motion. Breathing was
removed after hardware testing because every LVGL invalidation causes a logical
frame transfer on this RGB panel. Blinking and tap reactions preserve the
character without risking the flicker seen with continuous redraws.
The revised watch and companion screens still need device visual review.
