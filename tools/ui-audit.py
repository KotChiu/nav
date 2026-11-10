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
MAP_HUD = (BOARD / "src/map/MapHud.h").read_text()
MARKERS = (BOARD / "src/map/NavigationMarkers.h").read_text()
NAV_SPEED = (BOARD / "ui/LvglNavSpeed.h").read_text()
NAV_DISTANCE = (BOARD / "ui/LvglNavDistance.h").read_text()
NAV_POLICY = (BOARD / "ui/NavChromePolicy.h").read_text()
HOME_STACK_POLICY = (BOARD / "ui/HomeStackMotion.h").read_text()
PAGE_NAV = (BOARD / "src/pages/PageNavigation.h").read_text()
TOUCH = (BOARD / "src/pages/IncomingCall.h").read_text()
FUNCTIONS = (BOARD / "src/core/FunctionDeclarations.h").read_text()
NAV_DECL = (BOARD / "src/core/FunctionDeclarations.h").read_text()
MARKET_COMMANDS = (BOARD / "src/protocol/CommandReceiver.h").read_text()

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
assert '#define LV_FONT_MONTSERRAT_48 1' in (BOARD / "lv_conf.h").read_text()
assert 'watchTempLabel = MotoNavUI::label(homeWeatherCard, "--°",\n                    &lv_font_montserrat_28' in OVERLAY
assert 'LV_FONT_MONTSERRAT_64 1' not in (BOARD / "lv_conf.h").read_text()
# A connected device should feel like a product, not an engineering console.
# Time/forecast are blank until actual phone data arrives; their source is
# an implementation detail, not a homepage headline.
# Other pages may legitimately report offline phone data (e.g. music).
# This rule applies only to the home card, which must remain visually quiet.
home_start = OVERLAY.index("  homeOrbit[2] = MotoNavUI::panel(screen, 67")
home_end = OVERLAY.index("  homeHintLabel = MotoNavUI::label(screen,", home_start)
home_card = OVERLAY[home_start:home_end]
for noisy in ("TIME FROM PHONE", "SYNC PHONE TO SET TIME",
              "WEATHER FROM PHONE", "PHONE OFFLINE"):
    assert noisy not in home_card, noisy
home_tick_start = OVERLAY.index("  bool homeTextChanged = false;")
home_tick_end = OVERLAY.index("  MotoNavUI::rideUpdate(showRobot);", home_tick_start)
for noisy in ("TIME FROM PHONE", "SYNC PHONE TO SET TIME",
              "WEATHER FROM PHONE", "PHONE OFFLINE"):
    assert noisy not in OVERLAY[home_tick_start:home_tick_end], noisy
assert 'MotoNavUI::label(homeOrbit[2], ""' in OVERLAY
assert '"--:--"' in OVERLAY
assert 'homeStatusDot' in OVERLAY
assert '"LINKED" : "OFFLINE"' in OVERLAY
assert '"L %d°   H %d°"' in OVERLAY
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
# Map-first Carbon HUD: paint LVGL speed over a fully retained GFX fallback,
# and preserve all phone-provided map route/lanes/POI/navigation semantics.
assert '#include "../../ui/LvglNavSpeed.h"' in OVERLAY
assert "MotoNavUI::navSpeedCreate(screen)" in OVERLAY
assert "MotoNavUI::navSpeedUpdate" in OVERLAY
assert "mapModeActive()" in OVERLAY and "navMapCameraHudVisible()" in OVERLAY
assert "lv_font_montserrat_40" in NAV_SPEED
assert "NavSpeedCardWithCamera" in NAV_SPEED
assert "lv_font_montserrat_40" in NAV_DISTANCE
assert "lv_font_montserrat_28" in NAV_DISTANCE
assert "NavDistanceCard" in NAV_DISTANCE
assert "MotoNavUI::navDistanceCreate(screen)" in OVERLAY
assert "MotoNavUI::navDistanceUpdate" in OVERLAY
assert "nativeDistanceText = distText(navDist)" in OVERLAY
assert "if (speedKmh < 0) return;" in MAP_HUD
assert 'printCentered(String(speedKmh)' in MAP_HUD
assert "drawBigIconAt" in MARKERS and "printOutlined(dt" in MARKERS
assert "NavTurnCard" in MARKERS and "C_UI_PANEL" in MARKERS
assert "navChromeShowStatus" in OVERLAY and "navChromeShowStatus" in MAP_FRAME
assert "navLanesHudVisible()" in OVERLAY
assert "laneN > 0 && ringActive" in MAP_FRAME
assert "navMapChromeAllowed()" in OVERLAY
assert "obsAdd(MotoNavUI::NavMapChip" in MAP_FRAME
assert "obsAdd(MotoNavUI::NavLinkChip" in MAP_FRAME
assert "drawMapLights(cx, cy" in MAP_FRAME
assert "drawMapCams(cx, cy" in MAP_FRAME
assert "drawPlaceLabels(cx, cy" in MAP_FRAME
assert "drawMapLanes()" in MAP_FRAME
assert "navChromeFits" in NAV_POLICY
assert "bool navMapCameraHudVisible();" in NAV_DECL
# A fresh navigation screen must not claim map readiness or phone auth while
# both flags are false; state-colored line icons must match the text.
assert "justShown || !mapOverlay.statusInitialized" in MAP
assert "mapOverlayTintIcon(mapOverlay.mapIcon" in MAP
assert "mapOverlayTintIcon(mapOverlay.linkIcon" in MAP
assert 'actualMapData ? "MAP" : "SYNC"' in MAP
assert 'authenticatedPhone ? "LINK" : "OFF"' in MAP

# Market data safety is part of the visual contract: never reuse a prior
# symbol's CJK name/candles or mix chart periods during an asynchronous BLE
# refresh; don't call the present wall-clock minute a quote update time.
assert "PendingQuote pending[STOCK_MAX]" in MARKET_COMMANDS
assert "if (parsed != incoming) return;" in MARKET_COMMANDS
assert "if (!q.code.equalsIgnoreCase(pending[i].code))" in MARKET_COMMANDS
assert "name.showW = 0; name.showH = 0;" in MARKET_COMMANDS
assert "if (stockChartPeriod != newPeriod)" in MARKET_COMMANDS
assert "if (!complete" in MARKET_COMMANDS
assert "now - stockUpdatedAt" in OVERLAY
assert 'From iPhone / just received' in OVERLAY
assert 'updated %02d:%02d' not in OVERLAY

# Interactive Home card stack: the old clickable peeks remain usable, but
# vertical drag now owns the whole Home stack and leads to a real expanded
# three-card layout. Horizontal page snapshots must stay unchanged.
assert '#include "../../ui/HomeStackMotion.h"' in OVERLAY
assert "homeStackTouchMove(dx, dy, now)" in PAGE_NAV
assert "homeStackTouchEnd()" in PAGE_NAV
assert "homeStackCancel()" in PAGE_NAV
assert "bool homeStackTouchMove(int dx, int dy, unsigned long now);" in FUNCTIONS
assert "homeStackGestureActive" in OVERLAY
assert "&& !homeStackGestureActive" in OVERLAY
assert "lvglCancelPendingPress();" in OVERLAY
assert "homeStackTick(showHome);" in OVERLAY
assert "lv_obj_set_size(homeOrbit[i],r.w,r.h)" in OVERLAY
assert "homeStackProgress > 0.08f" in OVERLAY
assert "homeStackExpandedStyle" in OVERLAY
assert "homeBackSubtitle[0]" in OVERLAY and "homeBackSubtitle[1]" in OVERLAY
assert "homeBackChevron[0]" in OVERLAY and "homeBackChevron[1]" in OVERLAY
assert "stackReleaseTarget" in HOME_STACK_POLICY
assert "stackGestureEnabled" in HOME_STACK_POLICY
assert "StackOpen[3]" in HOME_STACK_POLICY
assert "if (callOn || pairOn || touchEat || fxOn) dragAbort();" in TOUCH
assert "pageForSwipe(page, dragHoriz, dragSign)" in PAGE_NAV

# Wallet-style stacked home and quote cards; prices/charts are never laid
# out in the same horizontal row as an unconstrained CJK stock name.
assert "MotoNavUI::panel(screen, 67, 115, 332, 245" in OVERLAY
assert "MotoNavUI::panel(screen, 82, 345, 145, 64" in OVERLAY
assert "MotoNavUI::panel(screen, 243, 345, 140, 64" in OVERLAY
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
