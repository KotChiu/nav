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
             ("Accent", "Background")):
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
print("MotoNav UI contract passed: readable palette, unified typography,"
      " no mascot, functional settings and existing route fallback.")
