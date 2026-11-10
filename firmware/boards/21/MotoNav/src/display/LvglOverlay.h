#pragma once

#include <lvgl.h>
#include "../../ui/VisualLanguage.h"
#include "../../ui/LvglIcons.h"
#include "../../ui/LvglMapOverlay.h"
#include "../../ui/LvglNavSpeed.h"
#include "../../ui/LvglNavDistance.h"
#include "../../ui/HomeStackMotion.h"
#include "../../ui/LvglSettingsPanel.h"
#include "../../ui/LvglRideDashboard.h"
#include "../../ui/LvglWeatherDashboard.h"

static inline lv_color_t lvglColor565(uint16_t color) {
  lv_color_t out;
  out.full = color;
  return out;
}

static lv_disp_draw_buf_t lvglDrawBuf;
static lv_disp_drv_t lvglDispDrv;
static lv_indev_t *lvglPointer = nullptr;
static volatile bool lvglOverlayDirty = false;
static bool lvglSnapshotting = false;
static void lvglSetSnapshotting(bool value) { lvglSnapshotting = value; }
volatile uint32_t lvglRenderCount = 0;
volatile uint32_t lvglHideChanges = 0;
volatile uint32_t lvglPosChanges = 0;
volatile uint32_t lvglTextChanges = 0;
volatile uint32_t lvglBgChanges = 0;
volatile uint32_t lvglTextColorChanges = 0;
static FlushRect lvglDirtyRects[FLUSH_RECT_MAX];
static int lvglDirtyRectCount = 0;
static lv_obj_t *musicButtons[3] = {nullptr, nullptr, nullptr};
static lv_obj_t *musicGlyphs[3] = {nullptr, nullptr, nullptr};
static lv_obj_t *musicPauseGlyph = nullptr;
static lv_obj_t *pageDotObjs[4] = {nullptr, nullptr, nullptr, nullptr};
// Settings is now a separate stateful controller in LvglSettingsPanel.h.
static lv_obj_t *homeTitleLabel = nullptr;
static lv_obj_t *homeStatusLabel = nullptr;
static lv_obj_t *homeHintLabel = nullptr;
static lv_obj_t *homeOrbit[3] = {};
static lv_obj_t *homeBackTitle[2] = {};
static lv_obj_t *homeBackSubtitle[2] = {};
static lv_obj_t *homeBackIcon[2] = {};
static lv_obj_t *homeBackChevron[2] = {};
static lv_obj_t *homeDivider = nullptr;
static lv_obj_t *homeDragHandle = nullptr;
// Gesture state lives alongside the LVGL scene; the existing page-swipe
// compositor only delegates Home vertical motion to this interaction.
static float homeStackProgress = 0.0f;
static float homeStackStartProgress = 0.0f;
static float homeStackAnimFrom = 0.0f;
static float homeStackAnimTo = 0.0f;
static float homeStackApplied = -1.0f;
static unsigned long homeStackAnimAt = 0;
static unsigned long homeStackRenderAt = 0;
static int homeStackLastDy = 0;
static bool homeStackGestureActive = false;
static bool homeStackSettling = false;
static bool homeStackExpandedStyle = false;
static lv_obj_t *homeWeatherCard = nullptr;
static lv_obj_t *homeStatusChip = nullptr;
static lv_obj_t *homeStatusDot = nullptr;
static lv_obj_t *musicHeaderLabel = nullptr;
static lv_obj_t *musicPlaceholder = nullptr;
static lv_obj_t *musicStatusLabel = nullptr;
static lv_obj_t *musicFooterLabel = nullptr;
static lv_obj_t *photoTitleLabel = nullptr;
static lv_obj_t *photoEmptyCard = nullptr;
static lv_obj_t *photoEmptyTitle = nullptr;
static lv_obj_t *photoEmptyHint = nullptr;
static lv_obj_t *watchTimeLabel = nullptr;
static lv_obj_t *watchDateLabel = nullptr;
static lv_obj_t *watchTempLabel = nullptr;
static lv_obj_t *watchForecastLabel = nullptr;
static uint32_t lastWatchEpoch = 0;
// Ride page is now an instrument dashboard; no animated expression state.
static lv_obj_t *batteryLabel = nullptr;
static lv_obj_t *stockTitleLabel = nullptr;
static lv_obj_t *stockStatusLabel = nullptr;
static lv_obj_t *stockList = nullptr;
static lv_obj_t *stockBackCards[2] = {};
static lv_obj_t *stockPrevButton = nullptr;
static lv_obj_t *stockNextButton = nullptr;
static lv_obj_t *stockCountLabel = nullptr;
static lv_obj_t *stockEmptyCard = nullptr;
static int stockFocused = 0;
static lv_obj_t *stockRows[STOCK_MAX] = {};
static lv_obj_t *stockCodeLabels[STOCK_MAX] = {};
static lv_obj_t *stockPriceLabels[STOCK_MAX] = {};
static lv_obj_t *stockChangeLabels[STOCK_MAX] = {};
static lv_obj_t *stockNameCanvases[STOCK_MAX] = {};
static lv_obj_t *stockChartCanvases[STOCK_MAX] = {};
static lv_color_t *stockNameBuffers[STOCK_MAX] = {};
static lv_color_t *stockChartBuffers[STOCK_MAX] = {};
static uint32_t stockLastRevision = UINT32_MAX;
static unsigned long stockLastStatusAt = 0;
static String stockLastStatus;
static void setLvglHidden(lv_obj_t *obj, bool hidden);
static int lvglLastPage = -1;
static bool lvglLastSliding = false;

static int lvglRectArea(const FlushRect &r) {
  return max(0, (int)r.x2 - r.x1 + 1) * max(0, (int)r.y2 - r.y1 + 1);
}

static FlushRect lvglRectUnion(const FlushRect &a, const FlushRect &b) {
  return {
    (int16_t)min(a.x1, b.x1), (int16_t)min(a.y1, b.y1),
    (int16_t)max(a.x2, b.x2), (int16_t)max(a.y2, b.y2)
  };
}

static bool lvglRectsNear(const FlushRect &a, const FlushRect &b) {
  const int gap = 4;
  return !(a.x2 + gap < b.x1 || b.x2 + gap < a.x1 ||
           a.y2 + gap < b.y1 || b.y2 + gap < a.y1);
}

static void lvglAddDirtyRect(const lv_area_t *area) {
  if (!area) return;
  FlushRect next = {
    (int16_t)constrain(area->x1, 0, LCD_WIDTH - 1),
    (int16_t)constrain(area->y1, 0, LCD_HEIGHT - 1),
    (int16_t)constrain(area->x2, 0, LCD_WIDTH - 1),
    (int16_t)constrain(area->y2, 0, LCD_HEIGHT - 1)
  };
  if (next.x2 < next.x1 || next.y2 < next.y1) return;

  bool merged;
  do {
    merged = false;
    for (int i = 0; i < lvglDirtyRectCount; i++) {
      if (!lvglRectsNear(lvglDirtyRects[i], next)) continue;
      next = lvglRectUnion(lvglDirtyRects[i], next);
      for (int j = i + 1; j < lvglDirtyRectCount; j++) {
        lvglDirtyRects[j - 1] = lvglDirtyRects[j];
      }
      lvglDirtyRectCount--;
      merged = true;
      break;
    }
  } while (merged);

  if (lvglDirtyRectCount < FLUSH_RECT_MAX) {
    lvglDirtyRects[lvglDirtyRectCount++] = next;
  } else {
    int best = 0, bestExtra = INT32_MAX;
    for (int i = 0; i < lvglDirtyRectCount; i++) {
      int extra = lvglRectArea(lvglRectUnion(lvglDirtyRects[i], next)) -
                  lvglRectArea(lvglDirtyRects[i]);
      if (extra < bestExtra) { bestExtra = extra; best = i; }
    }
    lvglDirtyRects[best] = lvglRectUnion(lvglDirtyRects[best], next);
  }
}

static void lvglFlushCb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
  (void)color_p;
  lvglRenderCount++;
  lvglAddDirtyRect(area);
  lvglOverlayDirty = true;
  lv_disp_flush_ready(drv);
}

static int lvglTakeDirtyRects(FlushRect *out, int maxCount) {
  int count = min(lvglDirtyRectCount, maxCount);
  for (int i = 0; i < count; i++) out[i] = lvglDirtyRects[i];
  return count;
}

static void lvglClearDirtyArea() {
  lvglDirtyRectCount = 0;
}

static void lvglTouchReadCb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
  (void)drv;
  data->state = touchDown && !dragging && !homeStackGestureActive
      ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
  if (touchDown) {
    float x = touchX1 - CX, y = touchY1 - CX;
    touchVectorToCanvas(x, y);
    data->point.x = lroundf(CX + x);
    data->point.y = lroundf(CX + y);
  }
}

static void musicButtonEventCb(lv_event_t *event) {
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
  int index = (int)(intptr_t)lv_event_get_user_data(event);
  int y = (coverValid && coverImg) ? 360 : 335;
  float ax = 1, ay = 0, bx = 0, by = 1;
  touchVectorToCanvas(ax, ay);
  touchVectorToCanvas(bx, by);
  float dx = (index - 1) * 100, dy = y - CX;
  musicTap(lroundf(CX + ax * dx + ay * dy),
           lroundf(CX + bx * dx + by * dy));
}

static lv_obj_t *makeMusicButton(int index) {
  lv_obj_t *button = lv_obj_create(lv_scr_act());
  lv_obj_set_size(button, 86, 86);
  lv_obj_set_style_radius(button, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(button, 1, 0);
  lv_obj_set_style_border_color(button, lvglColor565(C_UI_LINE), 0);
  lv_obj_set_style_bg_color(button, lvglColor565(C_UI_PANEL_2), 0);
  lv_obj_set_style_bg_color(button, lvglColor565(0x4BAE), LV_STATE_PRESSED);
  lv_obj_set_style_shadow_width(button, 0, 0);
  lv_obj_clear_flag(button, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(button, musicButtonEventCb, LV_EVENT_CLICKED, (void *)(intptr_t)index);

  musicGlyphs[index] = MotoNavUI::icon(button,
     index == 0 ? MotoNavUI::Icon::Previous :
     index == 2 ? MotoNavUI::Icon::Next : MotoNavUI::Icon::Play,
     31, 31, MotoNavUI::Text, 3);
  if (index == 1) {
    musicPauseGlyph = MotoNavUI::icon(button, MotoNavUI::Icon::Pause,
                                       31, 31, MotoNavUI::Background, 3);
    lv_obj_add_flag(musicPauseGlyph, LV_OBJ_FLAG_HIDDEN);
  }
  return button;
}

// Icon glyphs are paths, not missing LV_SYMBOL font characters. Update only
// when the enabled/disabled color changes to avoid per-frame line invalidation.
static void colorMusicGlyph(lv_obj_t *glyph, uint32_t rgb) {
  if (!glyph) return;
  lv_color_t wanted = MotoNavUI::color(rgb);
  const uint32_t count = lv_obj_get_child_cnt(glyph);
  for (uint32_t i = 0; i < count; ++i) {
    lv_obj_t *child = lv_obj_get_child(glyph, i);
    if (lv_color_to16(lv_obj_get_style_line_color(child, 0)) != lv_color_to16(wanted))
      lv_obj_set_style_line_color(child, wanted, 0);
  }
}

static void configureOpaqueLabel(lv_obj_t *label, int width, int height) {
  lv_obj_set_size(label, width, height);
  lv_obj_set_style_bg_opa(label, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(label, lv_color_black(), 0);
  lv_obj_set_style_pad_all(label, 0, 0);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
}

// Watch-face / home labels float over the dial drawn by drawWaiting(). An
// opaque background punches a black rectangle into it, so these stay
// transparent. drawWaiting() is repainted whenever their text changes, which
// erases the old glyphs (direct_mode has no background clear of its own).
static void configureTransparentLabel(lv_obj_t *label, int width, int height) {
  configureOpaqueLabel(label, width, height);
  lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, 0);
}

// All stock content is intentionally bounded within the focused 320x218 card.
// The previous 344px scrolling list let the chart and price escape at the
// bottom of the physical circle; the new component never grows horizontally.
#define STOCK_NAME_W 164
#define STOCK_NAME_H 28
#define STOCK_CHART_W 280
#define STOCK_CHART_H 47
#define STOCK_ROW_H 218
#define STOCK_PANEL_COLOR 0x191B21

static void stockCanvasRect(lv_color_t *buffer, int width, int x, int y,
                            int w, int h, lv_color_t color) {
  for (int yy = max(0, y); yy < min(STOCK_CHART_H, y + h); yy++)
    for (int xx = max(0, x); xx < min(width, x + w); xx++)
      buffer[yy * width + xx] = color;
}

static void stockCanvasHLine(lv_color_t *buffer, int y, lv_color_t color) {
  if (y < 0 || y >= STOCK_CHART_H) return;
  for (int x = 0; x < STOCK_CHART_W; x++) buffer[y * STOCK_CHART_W + x] = color;
}

static void updateStockNameCanvas(int index) {
  if (!stockNameCanvases[index] || !stockNameBuffers[index]) return;
  const lv_color_t bg = lv_color_hex(STOCK_PANEL_COLOR);
  for (int i = 0; i < STOCK_NAME_W * STOCK_NAME_H; i++)
    stockNameBuffers[index][i] = bg;

  const TextImg &im = imgs[13 + index];
  if (!im.show || !im.showW || !im.showH) {
    lv_obj_invalidate(stockNameCanvases[index]);
    return;
  }
  // Always fit the ENTIRE phone-supplied CJK name into the 164x28 window,
  // rather than simply cropping its rightmost characters like the old list.
  // The original 4bpp bitmap still comes from the real phone protocol.
  const int bpr = ((int)im.showW + 1) / 2;
  const int w = min((int)im.showW, STOCK_NAME_W);
  const int h = min((int)im.showH, STOCK_NAME_H);
  const int y0 = (STOCK_NAME_H - h) / 2;
  for (int y = 0; y < h; y++) {
    const int srcY = (int)((uint32_t)y * im.showH / h);
    for (int x = 0; x < w; x++) {
      const int srcX = (int)((uint32_t)x * im.showW / w);
      const uint8_t packed = im.show[srcY * bpr + (srcX >> 1)];
      const uint8_t nib = (srcX & 1) ? (packed & 0x0F) : (packed >> 4);
      if (!nib) continue;
      const uint8_t value = nib * 17;
      const uint16_t gray = ((value >> 3) << 11) |
                            ((value >> 2) << 5) | (value >> 3);
      stockNameBuffers[index][(y0 + y) * STOCK_NAME_W + x] = lvglColor565(gray);
    }
  }
  lv_obj_invalidate(stockNameCanvases[index]);
}

static void updateStockChartCanvas(int index) {
  if (!stockChartCanvases[index] || !stockChartBuffers[index]) return;
  const lv_color_t bg = lv_color_hex(STOCK_PANEL_COLOR);
  for (int i = 0; i < STOCK_CHART_W * STOCK_CHART_H; i++)
    stockChartBuffers[index][i] = bg;

  const StockQuote &q = stockQuotes[index];
  const int n = min((int)q.candleCount, 12);
  if (!n) {
    lv_obj_invalidate(stockChartCanvases[index]);
    return;
  }
  int low = 100, high = 0;
  for (int i = 0; i < n; i++) {
    low = min(low, (int)q.candles[i][3]);
    high = max(high, (int)q.candles[i][2]);
  }
  const int span = max(1, high - low);
  const int slot = max(10, (STOCK_CHART_W - 8) / n);
  const int bodyWidth = max(5, min(10, slot - 6));
  auto py = [&](int value) {
    return 4 + (100 - value) * (STOCK_CHART_H - 8) / 100;
  };
  const lv_color_t grid = lvglColor565(C_UI_LINE);
  stockCanvasHLine(stockChartBuffers[index], 13, grid);
  stockCanvasHLine(stockChartBuffers[index], 25, grid);
  stockCanvasHLine(stockChartBuffers[index], 37, grid);
  if (n) {
    int close = q.candles[n - 1][1];
    int closeY = py((close - low) * 100 / span);
    stockCanvasHLine(stockChartBuffers[index], closeY, lvglColor565(C_UI_ACCENT2));
  }
  for (int i = 0; i < n; i++) {
    int centerX = 4 + i * slot + slot / 2;
    int open = constrain(q.candles[i][0], low, high);
    int close = constrain(q.candles[i][1], low, high);
    int candleHigh = constrain(q.candles[i][2], low, high);
    int candleLow = constrain(q.candles[i][3], low, high);
    int openY = py((open - low) * 100 / span);
    int closeY = py((close - low) * 100 / span);
    int highY = py((candleHigh - low) * 100 / span);
    int lowY = py((candleLow - low) * 100 / span);
    uint16_t color = close >= open ? 0xF800 : 0x07E0;
    lv_color_t c = lvglColor565(color);
    stockCanvasRect(stockChartBuffers[index], STOCK_CHART_W, centerX, highY,
                    1, max(1, lowY - highY + 1), c);
    int top = min(openY, closeY);
    int height = max(2, abs(closeY - openY));
    stockCanvasRect(stockChartBuffers[index], STOCK_CHART_W,
                    centerX - bodyWidth / 2, top, bodyWidth, height, c);
  }
  lv_obj_invalidate(stockChartCanvases[index]);
}

static void stockShowFocused() {
  const int count = constrain(stockCount, 0, STOCK_MAX);
  if (count == 0) stockFocused = 0;
  else stockFocused = constrain(stockFocused, 0, count - 1);
  for (int i = 0; i < STOCK_MAX; ++i)
    setLvglHidden(stockRows[i], i != stockFocused || i >= count);
  if (stockCountLabel) {
    char text[22];
    snprintf(text, sizeof(text), count ? "%d / %d" : "0 / 0",
             count ? stockFocused + 1 : 0, count);
    const char *old = lv_label_get_text(stockCountLabel);
    if (!old || strcmp(old, text) != 0)
      lv_label_set_text(stockCountLabel, text);
  }
  if (stockEmptyCard) setLvglHidden(stockEmptyCard, count > 0);
  lv_obj_t *buttons[] = {stockPrevButton, stockNextButton};
  for (lv_obj_t *button : buttons) {
    if (!button) continue;
    const bool disabled = lv_obj_has_state(button, LV_STATE_DISABLED);
    if (count < 2 && !disabled) lv_obj_add_state(button, LV_STATE_DISABLED);
    else if (count >= 2 && disabled) lv_obj_clear_state(button, LV_STATE_DISABLED);
  }
}

static void stockStepEvent(lv_event_t *event) {
  const lv_event_code_t code = lv_event_get_code(event);
  if (code == LV_EVENT_PRESSED) {
    lvglCaptureTouch = true;
    return;
  }
  if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    lvglCaptureTouch = false;
    return;
  }
  if (code != LV_EVENT_CLICKED || page != PAGE_STOCKS ||
      sliding || pairOn || callOn || fxOn || stockCount < 2) return;
  const int delta = (int)(intptr_t)lv_event_get_user_data(event);
  const int count = constrain(stockCount, 1, STOCK_MAX);
  stockFocused = (stockFocused + delta + count) % count;
  stockShowFocused();
  needFlush = true;
}

// The page router sends vertical Home drags here BEFORE it starts a
// full-screen page transition. Only a touch beginning on the stack can
// activate it; active navigation and modal overlays always take priority.
bool homeStackTouchMove(int dx, int dy, unsigned long now) {
  const bool canDrag = MotoNavUI::stackGestureEnabled(
      lvglUiReady && page == PAGE_HOME, inNav,
      pairOn || callOn || fxOn || staleShown,
      sliding || dragging || slideSettling, lvglCaptureTouch);
  if (!canDrag) return false;
  if (!homeStackGestureActive) {
    if (touchGestureAxis != 2 || abs(dy) < DRAG_START) return false;
    if ((dy < 0 && homeStackProgress >= 0.999f) ||
        (dy > 0 && homeStackProgress <= 0.001f)) return false;
    float x = touchX0-CX, y = touchY0-CX;
    touchVectorToCanvas(x, y);
    if (!MotoNavUI::stackTouchInArea(lroundf(CX+x),lroundf(CX+y)))
      return false;
    homeStackGestureActive = true;
    homeStackSettling = false;
    homeStackStartProgress = homeStackProgress;
    homeStackLastDy = 0;
    // The very same press might have started on a clickable peek card.
    // Cancel the pending tap as soon as it becomes a deliberate drag.
    lvglCancelPendingPress();
  }
  homeStackLastDy = dy;
  homeStackProgress = MotoNavUI::stackDragProgress(
      homeStackStartProgress, dy);
  // The render loop samples this latest finger position at most once
  // every 30ms. Do not reset its throttle on each 8ms touch sample.
  (void)now;
  return true;
}

bool homeStackTouchEnd() {
  if (!homeStackGestureActive) return false;
  homeStackGestureActive = false;
  homeStackAnimFrom = homeStackProgress;
  homeStackAnimTo = MotoNavUI::stackReleaseTarget(
      homeStackProgress, homeStackStartProgress, homeStackLastDy);
  homeStackAnimAt = millis();
  homeStackSettling = fabsf(homeStackAnimFrom-homeStackAnimTo)>0.001f;
  if (!homeStackSettling) homeStackProgress = homeStackAnimTo;
  homeStackRenderAt = 0;
  return true;
}

void homeStackCancel() {
  if (!homeStackGestureActive && !homeStackSettling &&
      homeStackProgress <= 0.001f) return;
  homeStackGestureActive = false;
  homeStackSettling = false;
  homeStackProgress = 0.0f;
  homeStackAnimTo = 0.0f;
  homeStackApplied = -1.0f; // apply closed geometry when Home is visible again
  homeStackRenderAt = 0;
}

// The two cards still support direct tap entry, in either stack position.
static void homeStackCardEvent(lv_event_t *event) {
  if (lv_event_get_code(event) != LV_EVENT_CLICKED || page != PAGE_HOME ||
      inNav || dragging || sliding || pairOn || callOn || fxOn) return;
  int target = (int)(intptr_t)lv_event_get_user_data(event);
  page = target;
  pageAt = millis();
  forceFullFlush = true;
  if (target == PAGE_STOCKS) {
    // Match slideCommit's stock-page handshake: tapping a revealed card must
    // refresh real quotes, not open a permanently stale snapshot.
    pendingEvt = "T,S";
    stockDirty = true;
    drawStockPage();
  } else if (target == PAGE_COMPANION) {
    drawRobotPage();
  }
  needFlush = true;
}

static void createLvglOverlay() {
  lv_obj_t *screen = lv_scr_act();
  lv_obj_set_style_bg_opa(screen, LV_OPA_TRANSP, 0);
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

  for (int i = 0; i < 3; i++) {
    musicButtons[i] = makeMusicButton(i);
    lv_obj_add_flag(musicButtons[i], LV_OBJ_FLAG_HIDDEN);
  }

  for (int i = 0; i < 4; i++) {
    lv_obj_t *dot = lv_obj_create(screen);
    lv_obj_set_size(dot, 8, 8);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
    pageDotObjs[i] = dot;
  }

  // Dedicated four-section control center (display / ride / link / about).
  MotoNavUI::settingsCreate(screen);

  musicHeaderLabel = MotoNavUI::label(screen, "NOW PLAYING",
                        &lv_font_montserrat_20, MotoNavUI::Accent);
  lv_obj_align(musicHeaderLabel, LV_ALIGN_TOP_MID, 0, 58);
  lv_obj_set_style_text_letter_space(musicHeaderLabel, 3, 0);
  lv_obj_add_flag(musicHeaderLabel, LV_OBJ_FLAG_HIDDEN);
  musicStatusLabel = MotoNavUI::label(screen, "CONNECT YOUR PHONE",
                        &lv_font_montserrat_20, MotoNavUI::Muted);
  lv_obj_align(musicStatusLabel, LV_ALIGN_TOP_MID, 0, 275);
  lv_obj_add_flag(musicStatusLabel, LV_OBJ_FLAG_HIDDEN);
  musicFooterLabel = MotoNavUI::label(screen, "SWIPE DOWN  /  HOME",
                        &lv_font_montserrat_16, MotoNavUI::Faint);
  lv_obj_align(musicFooterLabel, LV_ALIGN_TOP_MID, 0, 402);
  lv_obj_add_flag(musicFooterLabel, LV_OBJ_FLAG_HIDDEN);

  musicPlaceholder = MotoNavUI::panel(screen, 169, 126, 128, 128, 64,
                                      MotoNavUI::SurfaceRaised);
  lv_obj_set_style_border_color(musicPlaceholder,
                               MotoNavUI::color(MotoNavUI::Line), 0);
  // Five quiet waveform bars are vector geometry, not a font-dependent
  // emoji or private-use glyph that might render as an empty square.
  const int heights[5] = {24, 45, 67, 45, 24};
  for (int i = 0; i < 5; ++i) {
    lv_obj_t *bar = lv_obj_create(musicPlaceholder);
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, 7, heights[i]);
    lv_obj_set_pos(bar, 27 + i * 17, (128 - heights[i]) / 2);
    lv_obj_set_style_radius(bar, 5, 0);
    lv_obj_set_style_bg_color(bar, MotoNavUI::color(MotoNavUI::Accent), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    MotoNavUI::noInteraction(bar);
  }
  lv_obj_add_flag(musicPlaceholder, LV_OBJ_FLAG_HIDDEN);

  // Apple Wallet-inspired focused card with two functional peeking tabs.
  // The old vertical overlap exposed only 18px of Markets: too little for
  // a gloved rider. Side-by-side back cards now each expose 49px of height
  // below the front card; all outer corners remain in the 466px safe circle.
  homeOrbit[0] = MotoNavUI::panel(screen, 243, 345, 140, 64, 23,
                                 MotoNavUI::SurfaceRaised);
  homeBackIcon[0] = MotoNavUI::icon(homeOrbit[0], MotoNavUI::Icon::Stocks,
                                     13, 21, MotoNavUI::Cool);
  homeBackTitle[0] = MotoNavUI::label(homeOrbit[0], "MARKETS",
                    &lv_font_montserrat_16, MotoNavUI::Muted);
  lv_obj_set_pos(homeBackTitle[0], 47, 27);
  homeBackSubtitle[0] = MotoNavUI::label(homeOrbit[0],
                    "QUOTES & CHARTS", &lv_font_montserrat_16, MotoNavUI::Muted);
  lv_obj_add_flag(homeBackSubtitle[0], LV_OBJ_FLAG_HIDDEN);
  homeBackChevron[0] = MotoNavUI::icon(homeOrbit[0],
                    MotoNavUI::Icon::ChevronRight, 200, 28, MotoNavUI::Muted);
  lv_obj_add_flag(homeBackChevron[0], LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(homeOrbit[0], LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(homeOrbit[0], homeStackCardEvent,
                      LV_EVENT_CLICKED, (void *)(intptr_t)PAGE_STOCKS);
  homeOrbit[1] = MotoNavUI::panel(screen, 82, 345, 145, 64, 23,
                                 MotoNavUI::Surface);
  homeBackIcon[1] = MotoNavUI::icon(homeOrbit[1], MotoNavUI::Icon::Ride,
                                     13, 21, MotoNavUI::Accent);
  homeBackTitle[1] = MotoNavUI::label(homeOrbit[1], "RIDE",
                    &lv_font_montserrat_16, MotoNavUI::Muted);
  lv_obj_set_pos(homeBackTitle[1], 47, 27);
  homeBackSubtitle[1] = MotoNavUI::label(homeOrbit[1],
                    "SPEED & TRIP", &lv_font_montserrat_16, MotoNavUI::Muted);
  lv_obj_add_flag(homeBackSubtitle[1], LV_OBJ_FLAG_HIDDEN);
  homeBackChevron[1] = MotoNavUI::icon(homeOrbit[1],
                    MotoNavUI::Icon::ChevronRight, 255, 28, MotoNavUI::Muted);
  lv_obj_add_flag(homeBackChevron[1], LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(homeOrbit[1], LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(homeOrbit[1], homeStackCardEvent,
                      LV_EVENT_CLICKED, (void *)(intptr_t)PAGE_COMPANION);
  // One calm, focused surface instead of a stack of technical status labels.
  // The two peeking pages remain reachable with 49px exposed touch bands.
  homeOrbit[2] = MotoNavUI::panel(screen, 67, 115, 332, 245, 29,
                                 MotoNavUI::SurfaceRaised);
  for (int i = 0; i < 3; i++)
    lv_obj_add_flag(homeOrbit[i], LV_OBJ_FLAG_HIDDEN);

  // The brand lives ABOVE the focused card, never repeated inside it.
  homeTitleLabel = MotoNavUI::label(screen, "MOTONAV",
                    &lv_font_montserrat_20, MotoNavUI::Accent);
  lv_obj_align(homeTitleLabel, LV_ALIGN_TOP_MID, 0, 66);
  lv_obj_set_style_text_letter_space(homeTitleLabel, 4, 0);
  lv_obj_add_flag(homeTitleLabel, LV_OBJ_FLAG_HIDDEN);

  // An unsynchronized clock is simply "--:--", not a developer instruction.
  // Date stays empty until the actual local date is known.
  watchDateLabel = MotoNavUI::label(homeOrbit[2], "",
                    &lv_font_montserrat_16, MotoNavUI::Muted);
  lv_obj_set_pos(watchDateLabel, 25, 28);
  lv_obj_set_size(watchDateLabel, 177, 25);
  lv_label_set_long_mode(watchDateLabel, LV_LABEL_LONG_CLIP);

  watchTimeLabel = MotoNavUI::label(homeOrbit[2], "--:--",
                    &lv_font_montserrat_48, MotoNavUI::Text);
  lv_obj_set_pos(watchTimeLabel, 20, 72);
  lv_obj_set_size(watchTimeLabel, 292, 75);
  lv_label_set_long_mode(watchTimeLabel, LV_LABEL_LONG_CLIP);
  lv_obj_set_style_text_letter_space(watchTimeLabel, 3, 0);

  // Small connection indicator; no oversized status banner.
  // This signal is diagnostic enough without dominating the watch face.
  homeStatusChip = lv_obj_create(homeOrbit[2]);
  lv_obj_remove_style_all(homeStatusChip);
  lv_obj_set_pos(homeStatusChip, 207, 19);
  lv_obj_set_size(homeStatusChip, 116, 34);
  MotoNavUI::noInteraction(homeStatusChip);
  homeStatusDot = lv_obj_create(homeStatusChip);
  lv_obj_remove_style_all(homeStatusDot);
  lv_obj_set_pos(homeStatusDot, 4, 12);
  lv_obj_set_size(homeStatusDot, 8, 8);
  lv_obj_set_style_radius(homeStatusDot, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(homeStatusDot, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(homeStatusDot,
                           MotoNavUI::color(MotoNavUI::Faint), 0);
  MotoNavUI::noInteraction(homeStatusDot);
  homeStatusLabel = MotoNavUI::label(homeStatusChip, "OFFLINE",
                    &lv_font_montserrat_16, MotoNavUI::Muted);
  lv_obj_set_pos(homeStatusLabel, 19, 7);
  lv_obj_set_size(homeStatusLabel, 96, 25);
  lv_label_set_long_mode(homeStatusLabel, LV_LABEL_LONG_CLIP);

  // One hairline divider, then a compact weather information row.
  homeDivider = lv_obj_create(homeOrbit[2]);
  lv_obj_remove_style_all(homeDivider);
  lv_obj_set_size(homeDivider, 282, 1);
  lv_obj_set_pos(homeDivider, 25, 161);
  lv_obj_set_style_bg_color(homeDivider, MotoNavUI::color(MotoNavUI::Line), 0);
  lv_obj_set_style_bg_opa(homeDivider, LV_OPA_COVER, 0);
  MotoNavUI::noInteraction(homeDivider);

  homeWeatherCard = MotoNavUI::panel(homeOrbit[2], 18, 174, 296, 62, 18,
                                     MotoNavUI::Surface);
  lv_obj_set_style_border_width(homeWeatherCard, 0, 0);
  MotoNavUI::icon(homeWeatherCard, MotoNavUI::Icon::Weather, 12, 19,
                  MotoNavUI::Cool);
  watchTempLabel = MotoNavUI::label(homeWeatherCard, "--°",
                    &lv_font_montserrat_28, MotoNavUI::Text);
  lv_obj_set_pos(watchTempLabel, 47, 14);
  lv_obj_set_size(watchTempLabel, 90, 39);
  lv_label_set_long_mode(watchTempLabel, LV_LABEL_LONG_CLIP);
  watchForecastLabel = MotoNavUI::label(homeWeatherCard, "",
                    &lv_font_montserrat_16, MotoNavUI::Muted);
  lv_obj_set_pos(watchForecastLabel, 146, 22);
  lv_obj_set_size(watchForecastLabel, 141, 26);
  lv_label_set_long_mode(watchForecastLabel, LV_LABEL_LONG_DOT);

  // Subtle drag affordance, not an instruction banner. It belongs to the
  // clock card and moves with it through the expanding/collapsing gesture.
  homeDragHandle = lv_obj_create(homeOrbit[2]);
  lv_obj_remove_style_all(homeDragHandle);
  lv_obj_set_pos(homeDragHandle, 148, 238);
  lv_obj_set_size(homeDragHandle, 36, 3);
  lv_obj_set_style_radius(homeDragHandle, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(homeDragHandle,
                           MotoNavUI::color(MotoNavUI::Faint), 0);
  lv_obj_set_style_bg_opa(homeDragHandle, LV_OPA_COVER, 0);
  MotoNavUI::noInteraction(homeDragHandle);

  homeHintLabel = MotoNavUI::label(screen, "SWIPE TO EXPLORE",
                    &lv_font_montserrat_16, MotoNavUI::Faint);
  lv_obj_add_flag(homeHintLabel, LV_OBJ_FLAG_HIDDEN);

  MotoNavUI::rideCreate(screen);
  MotoNavUI::weatherCreate(screen);

  // Electronic badge: full-bleed photos remain untouched; only the empty
  // state receives a tasteful type-led invitation.
  photoEmptyCard = MotoNavUI::panel(screen, 78, 129, 310, 230, 30);
  MotoNavUI::icon(photoEmptyCard, MotoNavUI::Icon::Photo, 142, 43,
                   MotoNavUI::Accent, 2);
  lv_obj_add_flag(photoEmptyCard, LV_OBJ_FLAG_HIDDEN);
  photoEmptyTitle = MotoNavUI::label(photoEmptyCard, "YOUR BADGE",
                             &lv_font_montserrat_28, MotoNavUI::Text);
  lv_obj_align(photoEmptyTitle, LV_ALIGN_TOP_MID, 0, 92);
  photoEmptyHint = MotoNavUI::label(photoEmptyCard,
                          "SEND AN IMAGE FROM YOUR PHONE",
                          &lv_font_montserrat_16, MotoNavUI::Muted);
  lv_obj_align(photoEmptyHint, LV_ALIGN_TOP_MID, 0, 144);
  photoTitleLabel = MotoNavUI::label(screen, "MOTONAV  /  BADGE",
                         &lv_font_montserrat_16, MotoNavUI::Accent);
  lv_obj_align(photoTitleLabel, LV_ALIGN_TOP_MID, 0, 62);
  lv_obj_add_flag(photoTitleLabel, LV_OBJ_FLAG_HIDDEN);

  // Markets = one focused quote with two visible card backs (Apple Wallet
  // visual grammar), rather than a scrollable list whose charts overflow.
  stockTitleLabel = MotoNavUI::label(screen, "MARKETS  /  TODAY",
                                      &lv_font_montserrat_28, MotoNavUI::Text);
  lv_obj_align(stockTitleLabel, LV_ALIGN_TOP_MID, 0, 49);
  lv_obj_add_flag(stockTitleLabel, LV_OBJ_FLAG_HIDDEN);

  stockStatusLabel = MotoNavUI::label(screen, "WAITING FOR PHONE QUOTES",
                                       &lv_font_montserrat_16, MotoNavUI::Muted);
  lv_obj_set_size(stockStatusLabel, 348, 25);
  lv_label_set_long_mode(stockStatusLabel, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_align(stockStatusLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(stockStatusLabel, LV_ALIGN_TOP_MID, 0, 93);
  lv_obj_add_flag(stockStatusLabel, LV_OBJ_FLAG_HIDDEN);

  stockBackCards[0] = MotoNavUI::panel(screen, 106, 362, 254, 50, 23,
                                        MotoNavUI::SurfaceRaised);
  MotoNavUI::icon(stockBackCards[0], MotoNavUI::Icon::Stocks, 20, 23,
                  MotoNavUI::Cool);
  lv_obj_t *backTwo = MotoNavUI::label(stockBackCards[0], "WATCHLIST",
                                  &lv_font_montserrat_16, MotoNavUI::Muted);
  lv_obj_set_pos(backTwo, 58, 29);
  stockBackCards[1] = MotoNavUI::panel(screen, 86, 345, 294, 49, 23,
                                        MotoNavUI::Surface);
  MotoNavUI::icon(stockBackCards[1], MotoNavUI::Icon::TrendUp, 22, 24,
                  MotoNavUI::Accent);
  lv_obj_t *backOne = MotoNavUI::label(stockBackCards[1], "NEXT POSITION",
                                  &lv_font_montserrat_16, MotoNavUI::Muted);
  lv_obj_set_pos(backOne, 58, 28);
  for (int i = 0; i < 2; ++i)
    lv_obj_add_flag(stockBackCards[i], LV_OBJ_FLAG_HIDDEN);

  stockList = lv_obj_create(screen);
  lv_obj_remove_style_all(stockList);
  lv_obj_set_size(stockList, 320, STOCK_ROW_H);
  lv_obj_set_pos(stockList, 73, 139);
  lv_obj_clear_flag(stockList, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(stockList, LV_OBJ_FLAG_HIDDEN);

  for (int i = 0; i < STOCK_MAX; i++) {
    lv_obj_t *row = MotoNavUI::panel(stockList, 0, 0, 320,
                                     STOCK_ROW_H, 26, MotoNavUI::SurfaceRaised);
    lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);
    stockRows[i] = row;

    stockCodeLabels[i] = MotoNavUI::label(row, "--",
                              &lv_font_montserrat_20, MotoNavUI::Text);
    lv_obj_set_pos(stockCodeLabels[i], 19, 15);
    lv_obj_set_size(stockCodeLabels[i], 167, 29);
    lv_label_set_long_mode(stockCodeLabels[i], LV_LABEL_LONG_DOT);
    lv_obj_add_flag(stockCodeLabels[i], LV_OBJ_FLAG_HIDDEN);

    stockNameBuffers[i] = (lv_color_t *)ps_malloc(
        STOCK_NAME_W * STOCK_NAME_H * sizeof(lv_color_t));
    if (stockNameBuffers[i]) {
      stockNameCanvases[i] = lv_canvas_create(row);
      lv_canvas_set_buffer(stockNameCanvases[i], stockNameBuffers[i],
                           STOCK_NAME_W, STOCK_NAME_H, LV_IMG_CF_TRUE_COLOR);
      lv_obj_set_pos(stockNameCanvases[i], 19, 16);
      lv_obj_clear_flag(stockNameCanvases[i],
                        LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
      updateStockNameCanvas(i);
    }

    // Price sits on its own line, not on top of a potentially long
    // CJK instrument name. One price/percent pair is always right-aligned.
    stockPriceLabels[i] = MotoNavUI::label(row, "--",
                                 &lv_font_montserrat_28, MotoNavUI::Text);
    lv_obj_set_pos(stockPriceLabels[i], 20, 49);
    lv_obj_set_size(stockPriceLabels[i], 279, 37);
    lv_label_set_long_mode(stockPriceLabels[i], LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(stockPriceLabels[i], LV_TEXT_ALIGN_RIGHT, 0);

    stockChangeLabels[i] = MotoNavUI::label(row, "--",
                                 &lv_font_montserrat_16, MotoNavUI::Muted);
    lv_obj_set_pos(stockChangeLabels[i], 21, 88);
    lv_obj_set_size(stockChangeLabels[i], 278, 27);
    lv_label_set_long_mode(stockChangeLabels[i], LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(stockChangeLabels[i], LV_TEXT_ALIGN_RIGHT, 0);

    stockChartBuffers[i] = (lv_color_t *)ps_malloc(
      STOCK_CHART_W * STOCK_CHART_H * sizeof(lv_color_t));
    if (stockChartBuffers[i]) {
      stockChartCanvases[i] = lv_canvas_create(row);
      lv_canvas_set_buffer(stockChartCanvases[i], stockChartBuffers[i],
                           STOCK_CHART_W, STOCK_CHART_H, LV_IMG_CF_TRUE_COLOR);
      lv_obj_set_pos(stockChartCanvases[i], 20, 123);
      lv_obj_clear_flag(stockChartCanvases[i],
                        LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
      updateStockChartCanvas(i);
    }
  }

  stockPrevButton = MotoNavUI::button(screen, 91, 314, 44, 38, "");
  lv_obj_del(lv_obj_get_child(stockPrevButton, 0));
  MotoNavUI::icon(stockPrevButton, MotoNavUI::Icon::ChevronLeft, 10, 6,
                  MotoNavUI::Text);
  lv_obj_add_event_cb(stockPrevButton, stockStepEvent, LV_EVENT_ALL,
                       (void *)(intptr_t)-1);
  stockNextButton = MotoNavUI::button(screen, 331, 314, 44, 38, "");
  lv_obj_del(lv_obj_get_child(stockNextButton, 0));
  MotoNavUI::icon(stockNextButton, MotoNavUI::Icon::ChevronRight, 10, 6,
                  MotoNavUI::Text);
  lv_obj_add_event_cb(stockNextButton, stockStepEvent, LV_EVENT_ALL,
                       (void *)(intptr_t)1);
  stockCountLabel = MotoNavUI::label(screen, "0 / 0",
                              &lv_font_montserrat_16, MotoNavUI::Muted);
  lv_obj_align(stockCountLabel, LV_ALIGN_TOP_MID, 0, 324);
  lv_obj_add_flag(stockPrevButton, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(stockNextButton, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(stockCountLabel, LV_OBJ_FLAG_HIDDEN);

  stockEmptyCard = MotoNavUI::panel(screen, 73, 139, 320, STOCK_ROW_H, 26);
  MotoNavUI::icon(stockEmptyCard, MotoNavUI::Icon::Stocks, 148, 38,
                  MotoNavUI::Accent);
  lv_obj_t *emptyTitle = MotoNavUI::label(stockEmptyCard, "NO QUOTES YET",
                                    &lv_font_montserrat_20, MotoNavUI::Text);
  lv_obj_align(emptyTitle, LV_ALIGN_TOP_MID, 0, 91);
  lv_obj_t *emptyDetail = MotoNavUI::label(stockEmptyCard,
       "SYNC YOUR WATCHLIST ON PHONE",
       &lv_font_montserrat_16, MotoNavUI::Muted);
  lv_obj_align(emptyDetail, LV_ALIGN_TOP_MID, 0, 134);
  lv_obj_add_flag(stockEmptyCard, LV_OBJ_FLAG_HIDDEN);

  MotoNavUI::mapOverlayCreate(screen);
  MotoNavUI::navSpeedCreate(screen);
  MotoNavUI::navDistanceCreate(screen);
}

static void lvglHybridInit() {
  lv_init();

  lv_color_t *drawBuffer = (lv_color_t *)gfx->getFramebuffer();
  lv_disp_draw_buf_init(&lvglDrawBuf, drawBuffer, nullptr, LCD_WIDTH * LCD_HEIGHT);
  lv_disp_drv_init(&lvglDispDrv);
  lvglDispDrv.hor_res = LCD_WIDTH;
  lvglDispDrv.ver_res = LCD_HEIGHT;
  lvglDispDrv.draw_buf = &lvglDrawBuf;
  lvglDispDrv.flush_cb = lvglFlushCb;
  lvglDispDrv.direct_mode = 1;
  lvglDispDrv.full_refresh = 0;
  lv_disp_drv_register(&lvglDispDrv);

  static lv_indev_drv_t indevDrv;
  lv_indev_drv_init(&indevDrv);
  indevDrv.type = LV_INDEV_TYPE_POINTER;
  indevDrv.read_cb = lvglTouchReadCb;
  lvglPointer = lv_indev_drv_register(&indevDrv);

  createLvglOverlay();
  batteryLabel = lv_label_create(lv_scr_act());
  // Percent only (no "BAT" prefix). Centered in the gap between the four
  // status arcs at 12 o'clock, so it never covers the ring.
  configureOpaqueLabel(batteryLabel, 64, 22);
  lv_obj_set_style_text_font(batteryLabel, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(batteryLabel, lv_color_white(), 0);
  lv_obj_set_pos(batteryLabel, CX - 32, 24);
  lvglLastPage = -1;
  lvglLastSliding = false;
  lvglUiReady = true;
}

static void lvglCancelPendingPress() {
  if (lvglPointer) lv_indev_reset(lvglPointer, nullptr);
}

static void setLvglHidden(lv_obj_t *obj, bool hidden) {
  if (!obj) return;
  bool isHidden = lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN);
  if (isHidden == hidden) return;
  lvglHideChanges++;
  if (hidden) lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

static void setLvglPos(lv_obj_t *obj, int x, int y) {
  if (!obj) return;
  if (lv_obj_get_x(obj) == x && lv_obj_get_y(obj) == y) return;
  lvglPosChanges++;
  lv_obj_set_pos(obj, x, y);
}

static bool setLvglLabelText(lv_obj_t *obj, const char *text) {
  if (!obj) return false;
  const char *current = lv_label_get_text(obj);
  if (current && text && strcmp(current, text) == 0) return false;
  lvglTextChanges++;
  lv_label_set_text(obj, text ? text : "");
  return true;
}

static void setLvglBgColor(lv_obj_t *obj, lv_color_t color) {
  if (!obj) return;
  if (lv_color_to16(lv_obj_get_style_bg_color(obj, 0)) == lv_color_to16(color)) return;
  lvglBgChanges++;
  lv_obj_set_style_bg_color(obj, color, 0);
}

static void setLvglTextColor(lv_obj_t *obj, lv_color_t color) {
  if (!obj) return;
  if (lv_color_to16(lv_obj_get_style_text_color(obj, 0)) == lv_color_to16(color)) return;
  lvglTextColorChanges++;
  lv_obj_set_style_text_color(obj, color, 0);
}

// Follow-finger transforms apply only to three LVGL cards. No page snapshot,
// full map compositing or extra PSRAM frame is allocated. Release animation
// uses a short ease-out at <=34 geometry updates/second.
static void homeStackTick(bool visible) {
  if (!visible) {
    homeStackCancel();
    return;
  }
  unsigned long now = millis();
  if (homeStackSettling && !homeStackGestureActive) {
    float t = MotoNavUI::stackClamp((now-homeStackAnimAt)/190.0f);
    float rem = 1.0f-t;
    float eased = 1.0f-rem*rem*rem;
    homeStackProgress = homeStackAnimFrom +
                        (homeStackAnimTo-homeStackAnimFrom)*eased;
    if (t >= 1.0f) {
      homeStackProgress = homeStackAnimTo;
      homeStackSettling = false;
    }
  }
  if (homeStackApplied >= 0 &&
      fabsf(homeStackProgress-homeStackApplied) < 0.003f) return;
  if (homeStackRenderAt && now-homeStackRenderAt < 30 &&
      (homeStackGestureActive || homeStackSettling)) return;
  homeStackRenderAt = now;
  homeStackApplied = homeStackProgress;
  const bool expandedLook = homeStackProgress > 0.40f;
  for (int i=0; i<3; ++i) {
    const MotoNavUI::HomeStackRect r=MotoNavUI::stackRect(i,homeStackProgress);
    if (lv_obj_get_x(homeOrbit[i])!=r.x || lv_obj_get_y(homeOrbit[i])!=r.y)
      lv_obj_set_pos(homeOrbit[i],r.x,r.y);
    if (lv_obj_get_width(homeOrbit[i])!=r.w ||
        lv_obj_get_height(homeOrbit[i])!=r.h)
      lv_obj_set_size(homeOrbit[i],r.w,r.h);
  }
  for (int i=0; i<2; ++i) {
    const MotoNavUI::HomeStackRect r=MotoNavUI::stackRect(i,homeStackProgress);
    setLvglPos(homeBackIcon[i],expandedLook?21:13,expandedLook?23:21);
    setLvglPos(homeBackTitle[i],expandedLook?63:47,expandedLook?18:27);
    setLvglPos(homeBackSubtitle[i],63,46);
    setLvglPos(homeBackChevron[i],r.w-44,28);
    setLvglHidden(homeBackSubtitle[i],!expandedLook);
    setLvglHidden(homeBackChevron[i],!expandedLook);
    if (homeStackExpandedStyle != expandedLook) {
      lv_obj_set_style_text_font(homeBackTitle[i],
        expandedLook ? &lv_font_montserrat_20 : &lv_font_montserrat_16,0);
      setLvglTextColor(homeBackTitle[i],
        MotoNavUI::color(expandedLook?MotoNavUI::Text:MotoNavUI::Muted));
    }
  }
  homeStackExpandedStyle=expandedLook;
  lv_obj_set_y(homeTitleLabel, lroundf(66-34*homeStackProgress));
  lv_obj_set_x(homeStatusChip,lroundf(207-4*homeStackProgress));
  const MotoNavUI::HomeStackRect clock=MotoNavUI::stackRect(
      MotoNavUI::StackClock,homeStackProgress);
  setLvglPos(homeDragHandle,(clock.w-36)/2,clock.h-9);
  setLvglHidden(homeDivider,homeStackProgress>0.08f);
  // The LVGL screen is transparent and uses the SAME RGB565 framebuffer as
  // GFX. Merely moving a card would leave its old pixels behind. Repaint the
  // real Home base BEFORE LVGL's visible scene is invalidated/refreshed below.
  // This also retains the existing LINK LOST ring and prevents ghost trails.
  if (!lvglSnapshotting) drawWaiting(waitSt);
}

static void lvglHybridTick() {
  if (!lvglUiReady) return;

  // Navigation keeps the top edge clear for the ring / N marker / speed HUD,
  // so the battery readout only shows on idle pages. No readout when no
  // battery is attached (batPct < 0).
  setLvglHidden(batteryLabel, batPct < 0 || pairOn || callOn || fxOn ||
                inNav || page == PAGE_PHOTO || page == PAGE_HOME);
  char batteryText[24];
  if (batPct >= 0) snprintf(batteryText, sizeof(batteryText), "%d%%", batPct);
  else snprintf(batteryText, sizeof(batteryText), "--");
  setLvglLabelText(batteryLabel, batteryText);
  const bool showWeather = page == PAGE_WEATHER && wxDetail == 0 &&
                      !sliding && !pairOn && !callOn && !fxOn;
  MotoNavUI::weatherUpdate(showWeather);
  bool showMusic = page == 2 && !sliding && !pairOn && !callOn && !fxOn;
  bool showSettings = page == 3 && !sliding && !pairOn && !callOn && !fxOn;
  bool showRobot = page == 4 && !sliding && !pairOn && !callOn && !fxOn;
  bool showStock = page == 5 && !sliding && !pairOn && !callOn && !fxOn;
  bool showHome = page == 0 && !inNav && !sliding && !pairOn && !callOn && !fxOn && !staleShown;
  homeStackTick(showHome);
  // Keep the GFX map, turn instructions, lanes and camera data untouched.
  // LVGL contributes a type-consistent speed panel only in live map mode.
  // Side status chips defer to lanes/camera so no informational overlay
  // can cover a higher-priority navigation cue.
  const bool liveNavHud = page == PAGE_HOME && inNav && !sliding &&
        !pairOn && !callOn && !fxOn && !staleShown && mapModeActive();
  const bool showLiveMap = liveNavHud && !cleanOn && navMapChromeAllowed();
  const bool mapHasData = navMapHasFrame();
  const bool cameraHudVisible = navMapCameraHudVisible();
  const bool showStatusChips = MotoNavUI::navChromeShowStatus(
      showLiveMap, cleanOn, navLanesHudVisible(), cameraHudVisible);
  MotoNavUI::mapOverlayUpdate(showStatusChips, mapHasData,
                               bleConnected && bleAuthed);
  MotoNavUI::navSpeedUpdate(liveNavHud && speedKmh >= 0, speedKmh,
                             cameraHudVisible,
                             cameraHudVisible && camLimit > 0 &&
                             speedKmh > camLimit);
  // Use the existing firmware formatter: no predicted distance, no duplicated
  // phone parsing. Keep the converted string stable across unchanged frames.
  const bool showNativeDistance = liveNavHud && !cruiseOn && navDist >= 0;
  static int lastNativeNavMetres = INT32_MIN;
  static String nativeDistanceText;
  if (showNativeDistance && navDist != lastNativeNavMetres) {
    lastNativeNavMetres = navDist;
    nativeDistanceText = distText(navDist);
  }
  MotoNavUI::navDistanceUpdate(showNativeDistance, nativeDistanceText.c_str(),
       navDist <= STAGE_NEAR_M);
  static bool lastShowHome = false;
  const bool homeJustShown = showHome && !lastShowHome;
  lastShowHome = showHome;
  bool infoDots = page >= 3 && page <= 5;
  bool showDots = false;
  int buttonY = (coverValid && coverImg) ? 360 : 335;
  const int dotOrder[] = {1, 0, 2};

  for (int i = 0; i < 3; i++) {
    int x = CX + (i - 1) * 100 - 43;
    setLvglPos(musicButtons[i], x, buttonY - 43);
    setLvglHidden(musicButtons[i], !showMusic);
    bool enabled = hidMusicMode ? (hidMediaReady() || appleMedia.controlReady(i)) : (bleConnected && bleAuthed && musicHave);
    const bool primary = enabled && i == 1;
    setLvglBgColor(musicButtons[i], lvglColor565(primary ? C_UI_ACCENT : enabled ? C_UI_PANEL_2 : C_UI_PANEL));
    if (i == 1) {
      setLvglHidden(musicGlyphs[1], musicPlaying);
      setLvglHidden(musicPauseGlyph, !musicPlaying);
    }
    const uint32_t glyphColor = primary ? MotoNavUI::Background :
                                enabled ? MotoNavUI::Text : MotoNavUI::Muted;
    colorMusicGlyph(musicGlyphs[i], glyphColor);
    if (i == 1) colorMusicGlyph(musicPauseGlyph, glyphColor);
  }

  MotoNavUI::settingsUpdate(showSettings);

  setLvglHidden(musicHeaderLabel, !showMusic || (coverValid && coverImg));
  setLvglHidden(musicPlaceholder, !showMusic || (coverValid && coverImg) ||
                ((bleConnected && bleAuthed) && musicHave));
  setLvglHidden(musicStatusLabel, !showMusic);
  setLvglHidden(musicFooterLabel, !showMusic);
  bool showPhotoEmpty = page == PAGE_PHOTO && !photoValid && !sliding &&
                        !pairOn && !callOn && !fxOn;
  setLvglHidden(photoEmptyCard, !showPhotoEmpty);
  setLvglHidden(photoTitleLabel, !showPhotoEmpty);
  if (showMusic) {
    const char *musicState = !(bleConnected && bleAuthed)
        ? "PHONE OFFLINE" : hidMusicMode && !appleMedia.available
        ? "OPEN A MUSIC APP" : !musicHave ? "NO PLAYER"
        : musicPlaying ? "PLAYING" : "PAUSED";
    setLvglLabelText(musicStatusLabel, musicState);
    setLvglTextColor(musicStatusLabel, lv_color_hex(
      musicPlaying ? MotoNavUI::Cool : MotoNavUI::Muted));
    lv_obj_align(musicStatusLabel, LV_ALIGN_TOP_MID, 0, 275);
  }

  bool homeTextChanged = false;
  setLvglHidden(homeTitleLabel, !showHome);
  setLvglHidden(homeStatusChip, !showHome);
  setLvglHidden(homeWeatherCard, !showHome || homeStackProgress > 0.08f);
  for (int i = 0; i < 3; ++i) setLvglHidden(homeOrbit[i], !showHome);
  setLvglHidden(homeHintLabel, true);
  if (showHome) {
    const bool linked = bleConnected && bleAuthed;
    const char *status = waitSt == WAIT_LOST ? "LOST" :
                         (linked ? "LINKED" : "OFFLINE");
    const uint32_t statusColor = linked ? C_UI_INFO :
                                 waitSt == WAIT_LOST ? C_UI_ACCENT2 : C_UI_DIM;
    if (setLvglLabelText(homeStatusLabel, status)) homeTextChanged = true;
    setLvglTextColor(homeStatusLabel, lvglColor565(statusColor));
    setLvglBgColor(homeStatusDot, lvglColor565(statusColor));
  }

  setLvglHidden(watchTimeLabel, !showHome);
  setLvglHidden(watchDateLabel, !showHome);
  // Temperature + forecast stay on the watch face, but are transparent and
  // pulled in from the dial edge so they never sit on the theme ring.
  setLvglHidden(watchTempLabel, !showHome);
  setLvglHidden(watchForecastLabel, !showHome);
  if (showHome) {
    uint32_t epoch = clockNowEpoch();
    if (epoch && epoch != lastWatchEpoch) {
      lastWatchEpoch = epoch;
      struct tm local {};
      if (clockLocalTime(local)) {
        char timeText[16], dateText[40];
        static const char *const months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
          "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        snprintf(timeText, sizeof(timeText), "%02d:%02d", local.tm_hour, local.tm_min);
        snprintf(dateText, sizeof(dateText), "%s %02d %s", WX_WEEK[local.tm_wday ? local.tm_wday : 7],
                 local.tm_mday, months[local.tm_mon % 12]);
        if (setLvglLabelText(watchTimeLabel, timeText)) homeTextChanged = true;
        if (setLvglLabelText(watchDateLabel, dateText)) homeTextChanged = true;
      }
    }
    char temperature[16];
    snprintf(temperature, sizeof(temperature), wxHave ? "%d°" : "--°", wxTemp);
    if (setLvglLabelText(watchTempLabel, temperature)) homeTextChanged = true;
    char forecast[36];
    if (wxHave && !wxHourly && wxN > 0)
      snprintf(forecast, sizeof(forecast), "L %d°   H %d°", wxFl[0], wxFh[0]);
    else if (wxHave && wxHourly && wxN > 0)
      snprintf(forecast, sizeof(forecast), "NEXT %d°", wxFh[0]);
    else snprintf(forecast, sizeof(forecast), "");
    if (setLvglLabelText(watchForecastLabel, forecast)) homeTextChanged = true;
  }

  // Transparent labels have no background to clear old glyphs in direct_mode,
  // and switching back from another page must not leave its pixels behind.
  // Repaint the canvas (black + status arcs) before LVGL redraws the text;
  // fillScreen() also invalidates every visible widget for us.
  if ((homeTextChanged || homeJustShown) && showHome && !sliding) drawWaiting(waitSt);

  MotoNavUI::rideUpdate(showRobot);

  setLvglHidden(stockList, !showStock || stockCount == 0);
  setLvglHidden(stockTitleLabel, !showStock);
  setLvglHidden(stockStatusLabel, !showStock);
  setLvglHidden(stockEmptyCard, !showStock || stockCount != 0);
  for (int i = 0; i < 2; ++i)
    setLvglHidden(stockBackCards[i], !showStock || stockCount == 0);
  setLvglHidden(stockPrevButton, !showStock || stockCount < 2);
  setLvglHidden(stockNextButton, !showStock || stockCount < 2);
  setLvglHidden(stockCountLabel, !showStock || stockCount == 0);
  if (showStock) {
    setLvglLabelText(stockTitleLabel,
      stockChartPeriod == 'M' ? "MARKETS  /  1M" : "MARKETS  /  TODAY");
    bool refresh = stockRevision != stockLastRevision || stockDirty;
    if (refresh) {
      stockLastRevision = stockRevision;
      stockDirty = false;
      for (int i = 0; i < STOCK_MAX; i++) {
        bool hasQuote = i < stockCount;
        if (!hasQuote) continue;
        const StockQuote &q = stockQuotes[i];
        const bool hasNameBitmap = imgs[13 + i].show &&
                         imgs[13 + i].showW && imgs[13 + i].showH &&
                         stockNameCanvases[i];
        lv_label_set_text(stockCodeLabels[i], q.code.c_str());
        setLvglHidden(stockCodeLabels[i], hasNameBitmap);
        setLvglHidden(stockNameCanvases[i], !hasNameBitmap);
        char price[24], change[24];
        snprintf(price, sizeof(price), "%.2f", q.price);
        float delta = fabsf(100 + q.changePct) > 0.001f ? q.price * q.changePct / (100 + q.changePct) : 0;
        snprintf(change, sizeof(change), "%+.2f  (%+.2f%%)", delta, q.changePct);
        lv_label_set_text(stockPriceLabels[i], price);
        lv_label_set_text(stockChangeLabels[i], change);
        uint32_t color = q.changePct > 0.0001f ? 0xFF5A68
          : q.changePct < -0.0001f ? 0x38D996 : 0x9AA3AD;
        lv_obj_set_style_text_color(stockChangeLabels[i], lv_color_hex(color), 0);
        updateStockNameCanvas(i);
        updateStockChartCanvas(i);
      }
      stockLastStatusAt = 0;
    }
    stockShowFocused();
    unsigned long now = millis();
    if (!stockLastStatusAt || now - stockLastStatusAt >= 5000) {
      stockLastStatusAt = now;
      String status;
      if (!stockCount) status = "Waiting for iPhone quotes";
      else if (!stockUpdatedAt) status = "Received quotes / age unknown";
      else {
        // stockUpdatedAt is millis() at #STK receipt, NOT an exchange timestamp.
        // The current wall clock must never be presented as the quote update time.
        const unsigned long age = now - stockUpdatedAt;
        const unsigned long minutes = age / 60000UL;
        char text[64];
        if (!(bleConnected && bleAuthed))
          snprintf(text, sizeof(text), "Phone offline / last sync %lu min ago", minutes);
        else if (age > 180000UL)
          snprintf(text, sizeof(text), "Quotes stale / %lu min ago", minutes);
        else if (age < 60000UL)
          snprintf(text, sizeof(text), "From iPhone / just received");
        else
          snprintf(text, sizeof(text), "From iPhone / %lu min ago", minutes);
        status = text;
      }
      if (status != stockLastStatus) {
        stockLastStatus = status;
        lv_label_set_text(stockStatusLabel, stockLastStatus.c_str());
      }
    }
  } else {
    stockLastRevision = UINT32_MAX;
    stockLastStatusAt = 0;
    for (int i = 0; i < STOCK_MAX; i++) setLvglHidden(stockRows[i], true);
  }

  int visibleDots = 3;
  int startX = CX - 20;
  int spacing = 20;
  for (int i = 0; i < 4; i++) {
    setLvglPos(pageDotObjs[i], startX + i * spacing - 4, 440);
    setLvglHidden(pageDotObjs[i], !showDots || i >= visibleDots);
    bool active = infoDots ? page == 3 + i : i < 3 && page == dotOrder[i];
    setLvglBgColor(pageDotObjs[i], lvglColor565(active ? C_UI_TEXT : C_UI_LINE));
  }

  if (page != lvglLastPage || sliding != lvglLastSliding) {
    lvglLastPage = page;
    lvglLastSliding = sliding;
  }

  // Canvas pages can clear the framebuffer underneath unchanged LVGL objects.
  // Invalidate visible top-level widgets so controls and labels are repainted.
  if (lvglCanvasRepainted) {
    lvglCanvasRepainted = false;
    lv_obj_t *screen = lv_scr_act();
    uint32_t childCount = lv_obj_get_child_cnt(screen);
    for (uint32_t i = 0; i < childCount; i++) {
      lv_obj_t *child = lv_obj_get_child(screen, i);
      if (child && !lv_obj_has_flag(child, LV_OBJ_FLAG_HIDDEN)) lv_obj_invalidate(child);
    }
  }

  // Snapshot preparation only needs a display refresh. Running the input
  // timers here would deliver the still-held finger to the destination page.
  if (lvglSnapshotting) lv_refr_now(nullptr);
  else lv_timer_handler();
  if (lvglOverlayDirty) {
    lvglOverlayDirty = false;
    needFlush = true;
    frameIsMap = false;
  }
}
