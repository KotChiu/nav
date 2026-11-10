#!/usr/bin/env python3
"""Small boardless design contract: consistent text family, useful settings,
safe fallback routing and readable foreground/background contrast.
The ESP32 hardware compile is the authoritative C++ test.
"""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
BOARD = ROOT / "firmware/boards/21/MotoNav"
OVERLAY = (BOARD / "src/display/LvglOverlay.h").read_text()
HOME = (BOARD / "src/pages/HomePage.h").read_text()
SETTINGS = (BOARD / "ui/LvglSettingsPanel.h").read_text()
RIDE = (BOARD / "ui/LvglRideDashboard.h").read_text()
TOKENS = (BOARD / "ui/VisualLanguage.h").read_text()
WEATHER = (BOARD / "ui/LvglWeatherDashboard.h").read_text()
ICONS = (BOARD / "ui/LvglIcons.h").read_text()
MAP = (BOARD / "ui/LvglMapOverlay.h").read_text()
MAP_FRAME = (BOARD / "src/map/MapFrame.h").read_text()

def srgb(c):
    t = c / 255
    return t / 12.92 if t <= 0.04045 else ((t + 0.055) / 1.055) ** 2.4

def luminosity(c):
    return sum(w * srgb((c >> shift) & 255)
               for w, shift in ((0.2126, 16), (0.7152, 8), (0.0722, 0)))

def contrast(a, b):
    x, y = sorted([luminosity(a), luminosity(b)], reverse=True)
    return (x + 0.05) / (y + 0.05)

colors = dict((k, int(v, 16)) for k, v in re.findall(
    r"static constexpr uint32_t (\w+) = 0x([0-9A-Fa-f]{6});", TOKENS))
for a, b in (("Text", "Background"), ("Text", "Surface"),
             ("Muted", "Background"), ("Muted", "Surface"),
             ("Accent", "Background"), ("Cool", "Surface"),
             ("Faint", "Surface")):
    assert contrast(colors[a], colors[b]) >= 4.5, (a, b, contrast(colors[a], colors[b]))

assert "WatchDigits.c" not in OVERLAY
assert "CompanionFace.h" not in OVERLAY
assert "RiderSprites.h" not in HOME
assert "drawRider(" not in HOME
assert "robotEyes" not in OVERLAY
assert "MotoNavUI::rideCreate" in OVERLAY
assert "MotoNavUI::weatherCreate" in OVERLAY
assert "MotoNavUI::settingsCreate" in OVERLAY
assert "lv_font_montserrat_48" in OVERLAY and "lv_font_montserrat_48" in RIDE
assert "lv_font_montserrat_48" in WEATHER
for name in ("DISPLAY", "RIDE", "LINK", "ABOUT"):
    assert name in SETTINGS
for state in ("screenBrightness", "setScreenDeg", "cleanOn", "progOn",
              "hidMusicMode", "brightnessSaveAt"):
    assert state in SETTINGS, state
for persisted in ("cleanmap", "progring", "mediahid"):
    assert f'"{persisted}"' in SETTINGS, persisted
assert "lvglCaptureTouch = true" in SETTINGS
assert "lvglCaptureTouch = false" in SETTINGS

# Every category has one vector icon; no emoji/bitmap icon font fallback.
for name in ("Map", "Route", "Weather", "Music", "Stocks", "Settings",
             "Screen", "Ride", "Link", "Photo", "ChevronLeft",
             "ChevronRight", "Play", "Pause", "Previous", "Next"):
    assert f"Icon::{name}" in ICONS
assert "MotoNavUI::mapOverlayCreate" in OVERLAY
assert "MotoNavUI::mapOverlayUpdate" in OVERLAY
assert "drawRasterMap(cx, cy" in MAP_FRAME
assert "drawRouteRange(" in MAP_FRAME
assert "drawMapLanes()" in MAP_FRAME
assert "drawMapBadge()" in MAP_FRAME
assert "lv_obj_move_foreground" in MAP

# Wallet-style stacked home and quote cards; prices/charts are never laid
# out in the same horizontal row as an unconstrained CJK stock name.
assert "MotoNavUI::panel(screen, 67, 115, 332, 245" in OVERLAY
assert "stockBackCards[0]" in OVERLAY and "stockBackCards[1]" in OVERLAY
assert "#define STOCK_NAME_W 164" in OVERLAY
assert "#define STOCK_CHART_W 280" in OVERLAY
assert "#define STOCK_ROW_H 218" in OVERLAY
assert "stockShowFocused()" in OVERLAY and "stockStepEvent" in OVERLAY
assert "lv_label_set_long_mode(stockPriceLabels[i], LV_LABEL_LONG_DOT)" in OVERLAY
assert "lv_obj_set_scroll_dir(stockList, LV_DIR_VER)" not in OVERLAY
assert "settings.depth[0]" in SETTINGS
assert "MotoNavUI::Icon::Weather" in OVERLAY
assert "MotoNavUI::Icon::Photo" in OVERLAY
print("MotoNav UI contracts passed: map-first navigation, vector icons,"
      " round-safe Wallet stacks, fixed stock bounds and real device data.")
