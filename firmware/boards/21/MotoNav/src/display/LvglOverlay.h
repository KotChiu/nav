#pragma once

#include <lvgl.h>
#include "../../resources/WatchDigits.c"
#include "../../ui/LvglHomeChrome.h"
#include "../../ui/LvglWeatherView.h"
#include "../../ui/LvglMusicStatusView.h"
#include "../../ui/LvglNavigationHud.h"
#include "../../ui/LvglNavigationCues.h"
#include "../../ui/LvglMapCanvas.h"
#include "../../ui/FrameOwnership.h"
#include "../../ui/LvglPageSlide.h"
#include "../../ui/LvglRasterAssets.h"

static inline lv_color_t lvglColor565(uint16_t color) {
  lv_color_t out;
  out.full = color;
  return out;
}

static MotoNavHomeChrome lvglHomeChrome;
static MotoNavWeatherView lvglWeatherView;
static MotoNavMusicStatusView lvglMusicView;
static MotoNavNavigationHud lvglMapHud;
static MotoNavNavigationCues lvglTurnCues;
static bool lvglNativeCueSupported() {
#if MOTONAV_NATIVE_NAV_CUES
  return motonavCueSupported(motonavCueKind(navDir.c_str()));
#else
  return false;
#endif
}
static MotoNavMapCanvas lvglMapCanvas;
static bool lvglMapCanvasCurrentFrame = false;
static bool lvglMapCanvasAllocationTried = false;
static uint16_t *lvglMapCanvasPixels() {
  return lvglMapCanvas.canvas ? lvglMapCanvas.pixels : nullptr;
}
static bool lvglMapCanvasStartFrame() {
#if MOTONAV_NATIVE_MAP_CANVAS
  if (!lvglMapCanvas.canvas && !lvglMapCanvasAllocationTried) {
    lvglMapCanvasAllocationTried = true;
    // Allocate *after* the existing page-transition snapshots have been
    // reserved at boot. A PSRAM-constrained device keeps its old map path.
    constexpr size_t bytes = (size_t)LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t);
    if (heap_caps_get_free_size(MALLOC_CAP_SPIRAM) > bytes + 256 * 1024) {
      uint16_t *pixels = (uint16_t *)ps_malloc(bytes);
      if (pixels) {
        memset(pixels, 0, bytes);
        if (!motonavMapCanvasCreate(lvglMapCanvas, lv_scr_act(), pixels,
                                    LCD_WIDTH, LCD_HEIGHT)) free(pixels);
        else Serial.printf("LVGL MAP: %u bytes PSRAM canvas ready\\n",
                           (unsigned)bytes);
      }
    }
    if (!lvglMapCanvas.canvas)
      Serial.println("LVGL MAP: allocation unavailable, using legacy path");
  }
  // Never let a map source alias its LVGL draw buffer, a DMA transport
  // snapshot or any already submitted frame.
  const uint16_t *source = lvglMapCanvas.pixels;
  if (!motonavFramePointersIndependent(gfx->getFramebuffer(), source,
                                       fbBuf[1], flushSrc)) return false;
  return motonavMapCanvasBegin(lvglMapCanvas);
#else
  return false;
#endif
}
static void lvglMapCanvasEndFrame() {
  motonavMapCanvasCommit(lvglMapCanvas);
  lvglMapCanvasCurrentFrame = true;
}
static void lvglMapCanvasFallback() {
  lvglMapCanvasCurrentFrame = false;
}
static bool lvglMapCanvasActive() { return lvglMapCanvas.drawing; }
static bool lvglMapCanvasLine(const float *x, const float *y, int n,
                              float width, uint16_t color, bool caps) {
  return motonavMapCanvasPolyline(lvglMapCanvas, x, y, n, width, color, caps);
}
static bool lvglMapCanvasPolygon(const float *x, const float *y,
                                 int n, uint16_t color) {
  return motonavMapCanvasPolygon(lvglMapCanvas, x, y, n, color);
}
static MotoNavLvglPageSlide lvglSwipeLayer;
// 4-bit images from the phone are shared between firmware and PC SDL.
// Image backing buffers are allocated lazily in PSRAM, not internal RAM.
static MotoNavRasterAsset lvglTextAssets[10];
// Phone-rendered grayscale slots: weather summary 3/4, music 5/6/12,
// QWeather rain/air 7/8 and alerts 9/10/11.
static const int lvglTextSlots[10] = {3, 4, 5, 6, 12, 7, 8, 9, 10, 11};
static MotoNavCoverAsset lvglAlbumCover;
static void *motonavRasterPsramAlloc(size_t bytes) { return ps_malloc(bytes); }
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
static lv_obj_t *musicButtonLabels[3] = {nullptr, nullptr, nullptr};
static lv_obj_t *pageDotObjs[4] = {nullptr, nullptr, nullptr, nullptr};
static lv_obj_t *settingsSlider = nullptr;
static lv_obj_t *settingsBrightnessLabel = nullptr;
static lv_obj_t *settingsTitleLabel = nullptr;
static lv_obj_t *settingsSubLabel = nullptr;
static lv_obj_t *settingsHintLabel = nullptr;
static lv_obj_t *settingsCard = nullptr;
static lv_obj_t *homeActions[2] = {};
static lv_obj_t *homeTitleLabel = nullptr;
static lv_obj_t *homeStatusLabel = nullptr;
static lv_obj_t *homeDeviceLabel = nullptr;
static lv_obj_t *homeHintLabel = nullptr;
static lv_obj_t *homeStatusDot = nullptr;
static lv_obj_t *watchTimeLabel = nullptr;
static lv_obj_t *watchDateLabel = nullptr;
static lv_obj_t *watchTempLabel = nullptr;
static lv_obj_t *watchForecastLabel = nullptr;
static uint32_t lastWatchEpoch = 0;
static lv_obj_t *robotHalo = nullptr;
static lv_obj_t *robotBody = nullptr;
static lv_obj_t *robotFace = nullptr;
static lv_obj_t *robotEyes[2] = {nullptr, nullptr};
static lv_obj_t *robotTitleLabel = nullptr;
static lv_obj_t *robotMoodLabel = nullptr;
static unsigned long robotLastAnimAt = 0;
static unsigned long robotReactUntil = 0;
static bool robotLastBlink = false;
static bool robotLastOnline = false;
static bool robotLastReacting = false;
static String robotLastMood;
static lv_obj_t *batteryLabel = nullptr;
static lv_obj_t *stockTitleLabel = nullptr;
static lv_obj_t *stockStatusLabel = nullptr;
static lv_obj_t *stockList = nullptr;
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
  data->state = touchDown && !dragging ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
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

static void robotTapEventCb(lv_event_t *event) {
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
  robotReactUntil = millis() + 2200;
  robotLastAnimAt = 0;
}

static void saveBrightness() {
  Preferences pr;
  pr.begin("motonav", false);
  pr.putInt("bright", screenBrightness);
  pr.end();
}

static void brightnessEventCb(lv_event_t *event) {
  lv_event_code_t code = lv_event_get_code(event);
  if (code == LV_EVENT_PRESSED) {
    lvglCaptureTouch = true;
  } else if (code == LV_EVENT_VALUE_CHANGED) {
    screenBrightness = constrain((int)lv_slider_get_value(settingsSlider), 5, 180);
    panel->setBrightness(screenBrightness);
    lv_label_set_text_fmt(settingsBrightnessLabel, "%d%%",
                          (screenBrightness * 100 + 92) / 180);
    lv_obj_invalidate(settingsSlider);
  } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    lvglCaptureTouch = false;
    saveBrightness();
  }
}

static lv_obj_t *makeMusicButton(int index) {
  lv_obj_t *button = lv_obj_create(lv_scr_act());
  lv_obj_set_size(button, 86, 86);
  lv_obj_set_style_radius(button, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(button, 1, 0);
  lv_obj_set_style_border_color(button, lvglColor565(C_UI_ACCENT), 0);
  lv_obj_set_style_bg_color(button, lvglColor565(C_UI_PANEL_2), 0);
  lv_obj_set_style_bg_color(button, lvglColor565(C_UI_ACCENT), LV_STATE_PRESSED);
  lv_obj_set_style_shadow_width(button, 0, 0);
  lv_obj_clear_flag(button, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(button, musicButtonEventCb, LV_EVENT_CLICKED, (void *)(intptr_t)index);

  lv_obj_t *label = lv_label_create(button);
  lv_obj_set_style_text_color(label, lv_color_white(), 0);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_28, 0);
  lv_obj_center(label);
  musicButtonLabels[index] = label;
  return button;
}

static void configureOpaqueLabel(lv_obj_t *label, int width, int height) {
  lv_obj_set_size(label, width, height);
  lv_obj_set_style_bg_opa(label, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(label, lv_color_black(), 0);
  lv_obj_set_style_pad_all(label, 0, 0);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
}

#define STOCK_NAME_W 180
#define STOCK_NAME_H 28
#define STOCK_CHART_W 306
#define STOCK_CHART_H 52
#define STOCK_ROW_H 124
#define STOCK_PANEL_COLOR 0x000000

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
  const int bpr = (im.showW + 1) / 2;
  const int y0 = im.showH < STOCK_NAME_H ? (STOCK_NAME_H - im.showH) / 2 : 0;
  const int w = min((int)im.showW, STOCK_NAME_W);
  const int h = min((int)im.showH, STOCK_NAME_H);
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      uint8_t packed = im.show[y * bpr + (x >> 1)];
      uint8_t nib = (x & 1) ? (packed & 0x0F) : (packed >> 4);
      if (!nib) continue;
      uint8_t value = nib * 17;
      uint16_t gray = ((value >> 3) << 11) | ((value >> 2) << 5) | (value >> 3);
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

static void homeActionEvent(lv_event_t *event) {
  if (lv_event_get_code(event) != LV_EVENT_CLICKED || sliding) return;
  int target = (int)(intptr_t)lv_event_get_user_data(event);
  showPageAnimated(target, -1, 0);
}

static void createLvglOverlay() {
  lv_obj_t *screen = lv_scr_act();
  lv_obj_set_style_bg_opa(screen, LV_OPA_TRANSP, 0);
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
  // The map Canvas is created lazily after slide buffers are reserved.
  // This avoids starving startup UI and preserves a low-memory fallback.
  // Home ring is now a native LVGL layer and does not rely on Canvas pixels.
  motonavHomeChromeCreate(lvglHomeChrome, screen, CX, C_UI_ACCENT, C_UI_ACCENT2);
  motonavWeatherViewCreate(lvglWeatherView, screen);
  motonavMusicStatusCreate(lvglMusicView, screen);
  motonavNavigationHudCreate(lvglMapHud, screen);
  motonavNavigationCuesCreate(lvglTurnCues, screen);
  // The existing PSRAM snapshots will be shown as two LVGL image objects.
  // It is not another framebuffer allocation.
  motonavLvglSlideCreate(lvglSwipeLayer, screen, LCD_WIDTH, LCD_HEIGHT);
  for (int i = 0; i < 10; ++i)
    motonavRasterCreate(lvglTextAssets[i], screen, motonavRasterPsramAlloc, free);
  motonavCoverCreate(lvglAlbumCover, screen, motonavRasterPsramAlloc, free);

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

  settingsCard = lv_obj_create(screen);
  lv_obj_set_size(settingsCard, 340, 190);
  lv_obj_set_pos(settingsCard, CX - 170, 120);
  lv_obj_set_style_radius(settingsCard, 8, 0);
  lv_obj_set_style_border_width(settingsCard, 1, 0);
  lv_obj_set_style_border_color(settingsCard, lvglColor565(C_UI_LINE), 0);
  lv_obj_set_style_bg_opa(settingsCard, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(settingsCard, lvglColor565(C_UI_PANEL), 0);
  lv_obj_clear_flag(settingsCard, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(settingsCard, LV_OBJ_FLAG_HIDDEN);

  settingsSlider = lv_slider_create(settingsCard);
  lv_obj_set_size(settingsSlider, 300, 28);
  lv_obj_set_pos(settingsSlider, 20, 84);
  lv_slider_set_range(settingsSlider, 5, 180);
  lv_slider_set_value(settingsSlider, screenBrightness, LV_ANIM_OFF);
  lv_obj_set_style_radius(settingsSlider, LV_RADIUS_CIRCLE, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(settingsSlider, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(settingsSlider, lvglColor565(C_UI_LINE), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(settingsSlider, LV_OPA_COVER, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(settingsSlider, lvglColor565(C_UI_ACCENT), LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(settingsSlider, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
  lv_obj_set_style_border_width(settingsSlider, 0, LV_PART_KNOB);
  lv_obj_set_style_pad_all(settingsSlider, 6, LV_PART_KNOB);
  lv_obj_add_event_cb(settingsSlider, brightnessEventCb, LV_EVENT_ALL, nullptr);
  lv_obj_add_flag(settingsSlider, LV_OBJ_FLAG_HIDDEN);

  settingsBrightnessLabel = lv_label_create(settingsCard);
  lv_obj_set_size(settingsBrightnessLabel, 92, 42);
  lv_obj_set_style_bg_opa(settingsBrightnessLabel, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(settingsBrightnessLabel, lvglColor565(C_UI_PANEL_2), 0);
  lv_obj_set_style_radius(settingsBrightnessLabel, 8, 0);
  lv_obj_set_style_pad_top(settingsBrightnessLabel, 8, 0);
  lv_obj_set_style_text_color(settingsBrightnessLabel, lv_color_white(), 0);
  lv_obj_set_style_text_font(settingsBrightnessLabel, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_align(settingsBrightnessLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_text_fmt(settingsBrightnessLabel, "%d%%", (screenBrightness * 100 + 92) / 180);
  lv_obj_set_pos(settingsBrightnessLabel, 124, 132);
  lv_obj_add_flag(settingsBrightnessLabel, LV_OBJ_FLAG_HIDDEN);

  settingsTitleLabel = lv_label_create(screen);
  lv_obj_set_style_text_color(settingsTitleLabel, lv_color_white(), 0);
  lv_obj_set_style_text_font(settingsTitleLabel, &lv_font_montserrat_28, 0);
  lv_label_set_text(settingsTitleLabel, "Settings");
  lv_obj_align(settingsTitleLabel, LV_ALIGN_TOP_MID, 0, 52);
  lv_obj_add_flag(settingsTitleLabel, LV_OBJ_FLAG_HIDDEN);

  settingsSubLabel = lv_label_create(screen);
  lv_obj_set_style_text_color(settingsSubLabel, lvglColor565(C_UI_SUB), 0);
  lv_obj_set_style_text_font(settingsSubLabel, &lv_font_montserrat_20, 0);
  lv_label_set_text(settingsSubLabel, "Display brightness");
  lv_obj_align(settingsSubLabel, LV_ALIGN_TOP_MID, 0, 132);
  lv_obj_add_flag(settingsSubLabel, LV_OBJ_FLAG_HIDDEN);

  settingsHintLabel = lv_label_create(screen);
  lv_obj_set_style_text_color(settingsHintLabel, lvglColor565(C_UI_DIM), 0);
  lv_obj_set_style_text_font(settingsHintLabel, &lv_font_montserrat_16, 0);
  lv_label_set_text(settingsHintLabel, "Swipe down for home");
  lv_obj_align(settingsHintLabel, LV_ALIGN_TOP_MID, 0, 400);
  lv_obj_add_flag(settingsHintLabel, LV_OBJ_FLAG_HIDDEN);

  for (int i = 0; i < 2; ++i) {
    homeActions[i] = lv_btn_create(screen);
    lv_obj_set_size(homeActions[i], 142, 56);
    lv_obj_set_pos(homeActions[i], CX - 150 + i * 158, 352);
    lv_obj_set_style_radius(homeActions[i], 8, 0);
    lv_obj_set_style_bg_color(homeActions[i], lvglColor565(C_UI_PANEL), 0);
    lv_obj_set_style_border_width(homeActions[i], 1, 0);
    lv_obj_set_style_border_color(homeActions[i], lvglColor565(C_UI_ACCENT), 0);
    lv_obj_set_style_shadow_width(homeActions[i], 0, 0);
    lv_obj_add_event_cb(homeActions[i], homeActionEvent, LV_EVENT_CLICKED,
                        (void *)(intptr_t)(i ? PAGE_MUSIC : PAGE_WEATHER));
    lv_obj_t *label = lv_label_create(homeActions[i]);
    lv_label_set_text(label, i ? LV_SYMBOL_AUDIO "  MUSIC" : "WEATHER");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(label, lvglColor565(C_UI_TEXT), 0);
    lv_obj_center(label);
    lv_obj_add_flag(homeActions[i], LV_OBJ_FLAG_HIDDEN);
  }
  homeTitleLabel = lv_label_create(screen);
  lv_obj_set_style_text_color(homeTitleLabel, lv_color_white(), 0);
  lv_obj_set_style_text_font(homeTitleLabel, &lv_font_montserrat_20, 0);
  lv_label_set_text(homeTitleLabel, "MOTO / NAV");
  lv_obj_align(homeTitleLabel, LV_ALIGN_TOP_MID, 0, 52);
  lv_obj_add_flag(homeTitleLabel, LV_OBJ_FLAG_HIDDEN);

  homeStatusDot = lv_obj_create(screen);
  lv_obj_set_size(homeStatusDot, 12, 12);
  lv_obj_set_style_radius(homeStatusDot, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(homeStatusDot, 0, 0);
  lv_obj_clear_flag(homeStatusDot, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(homeStatusDot, LV_ALIGN_TOP_MID, -91, 207);
  lv_obj_add_flag(homeStatusDot, LV_OBJ_FLAG_HIDDEN);

  homeStatusLabel = lv_label_create(screen);
  lv_obj_set_style_text_font(homeStatusLabel, &lv_font_montserrat_20, 0);
  configureOpaqueLabel(homeStatusLabel, 220, 30);
  lv_obj_align(homeStatusLabel, LV_ALIGN_TOP_MID, 14, 198);
  lv_obj_add_flag(homeStatusLabel, LV_OBJ_FLAG_HIDDEN);

  homeDeviceLabel = lv_label_create(screen);
  lv_obj_set_style_text_color(homeDeviceLabel, lvglColor565(C_UI_SUB), 0);
  lv_obj_set_style_text_font(homeDeviceLabel, &lv_font_montserrat_16, 0);
  configureOpaqueLabel(homeDeviceLabel, 300, 28);
  lv_obj_align(homeDeviceLabel, LV_ALIGN_TOP_MID, 0, 330);
  lv_obj_add_flag(homeDeviceLabel, LV_OBJ_FLAG_HIDDEN);

  homeHintLabel = lv_label_create(screen);
  lv_obj_set_style_text_color(homeHintLabel, lvglColor565(C_UI_DIM), 0);
  lv_obj_set_style_text_font(homeHintLabel, &lv_font_montserrat_16, 0);
  lv_label_set_text(homeHintLabel, "");
  lv_obj_align(homeHintLabel, LV_ALIGN_TOP_MID, 0, 420);
  lv_obj_add_flag(homeHintLabel, LV_OBJ_FLAG_HIDDEN);

  watchTimeLabel = lv_label_create(screen);
  lv_obj_set_style_text_color(watchTimeLabel, lv_color_white(), 0);
  lv_obj_set_style_text_font(watchTimeLabel, &motonav_watch_digits, 0);
  configureOpaqueLabel(watchTimeLabel, 340, 82);
  lv_label_set_text(watchTimeLabel, "00:00");
  lv_obj_align(watchTimeLabel, LV_ALIGN_TOP_MID, 0, 108);
  lv_obj_add_flag(watchTimeLabel, LV_OBJ_FLAG_HIDDEN);

  watchDateLabel = lv_label_create(screen);
  lv_obj_set_style_text_color(watchDateLabel, lvglColor565(C_UI_SUB), 0);
  lv_obj_set_style_text_font(watchDateLabel, &lv_font_montserrat_16, 0);
  configureOpaqueLabel(watchDateLabel, 240, 28);
  lv_label_set_text(watchDateLabel, "WAITING FOR PHONE TIME");
  lv_obj_align(watchDateLabel, LV_ALIGN_TOP_MID, 0, 76);
  lv_obj_add_flag(watchDateLabel, LV_OBJ_FLAG_HIDDEN);

  watchTempLabel = lv_label_create(screen);
  lv_obj_set_style_text_color(watchTempLabel, lv_color_white(), 0);
  lv_obj_set_style_text_font(watchTempLabel, &lv_font_montserrat_40, 0);
  configureOpaqueLabel(watchTempLabel, 120, 52);
  lv_label_set_text(watchTempLabel, "--°");
  lv_obj_align(watchTempLabel, LV_ALIGN_TOP_MID, 89, 240);
  lv_obj_add_flag(watchTempLabel, LV_OBJ_FLAG_HIDDEN);

  watchForecastLabel = lv_label_create(screen);
  lv_obj_set_style_text_color(watchForecastLabel, lvglColor565(C_UI_SUB), 0);
  lv_obj_set_style_text_font(watchForecastLabel, &lv_font_montserrat_20, 0);
  configureOpaqueLabel(watchForecastLabel, 310, 34);
  lv_label_set_text(watchForecastLabel, "WEATHER FROM PHONE");
  lv_obj_align(watchForecastLabel, LV_ALIGN_TOP_MID, 0, 304);
  lv_obj_add_flag(watchForecastLabel, LV_OBJ_FLAG_HIDDEN);

  robotHalo = lv_obj_create(screen);
  lv_obj_set_size(robotHalo, 290, 290);
  lv_obj_set_pos(robotHalo, CX - 145, 86);
  lv_obj_set_style_radius(robotHalo, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(robotHalo, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(robotHalo, 3, 0);
  lv_obj_set_style_border_color(robotHalo, lvglColor565(C_UI_ACCENT), 0);
  lv_obj_clear_flag(robotHalo, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(robotHalo, LV_OBJ_FLAG_HIDDEN);

  robotBody = lv_obj_create(screen);
  lv_obj_set_size(robotBody, 224, 238);
  lv_obj_set_pos(robotBody, CX - 112, 112);
  lv_obj_set_style_radius(robotBody, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(robotBody, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(robotBody, lv_color_hex(0xF9FAF7), 0);
  lv_obj_set_style_bg_grad_color(robotBody, lv_color_hex(0xD8E2E1), 0);
  lv_obj_set_style_bg_grad_dir(robotBody, LV_GRAD_DIR_VER, 0);
  lv_obj_set_style_border_width(robotBody, 0, 0);
  lv_obj_set_style_shadow_width(robotBody, 0, 0);
  lv_obj_clear_flag(robotBody, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(robotBody, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(robotBody, robotTapEventCb, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_flag(robotBody, LV_OBJ_FLAG_HIDDEN);

  robotFace = lv_obj_create(robotBody);
  lv_obj_set_size(robotFace, 160, 100);
  lv_obj_align(robotFace, LV_ALIGN_TOP_MID, 0, 64);
  lv_obj_set_style_bg_opa(robotFace, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(robotFace, 0, 0);
  lv_obj_set_style_pad_all(robotFace, 0, 0);
  lv_obj_clear_flag(robotFace, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(robotFace, LV_OBJ_FLAG_CLICKABLE);

  for (int i = 0; i < 2; i++) {
    robotEyes[i] = lv_obj_create(robotFace);
    lv_obj_set_size(robotEyes[i], 28, 50);
    lv_obj_set_pos(robotEyes[i], i == 0 ? 37 : 95, 25);
    lv_obj_set_style_radius(robotEyes[i], LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(robotEyes[i], LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(robotEyes[i], lv_color_hex(0x11171B), 0);
    lv_obj_set_style_border_width(robotEyes[i], 0, 0);
    lv_obj_clear_flag(robotEyes[i], LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(robotEyes[i], LV_OBJ_FLAG_CLICKABLE);
  }

  robotTitleLabel = lv_label_create(screen);
  lv_obj_set_style_text_color(robotTitleLabel, lvglColor565(C_UI_SUB), 0);
  lv_obj_set_style_text_font(robotTitleLabel, &lv_font_montserrat_16, 0);
  lv_label_set_text(robotTitleLabel, "COMPANION");
  lv_obj_align(robotTitleLabel, LV_ALIGN_TOP_MID, 0, 48);
  lv_obj_add_flag(robotTitleLabel, LV_OBJ_FLAG_HIDDEN);

  robotMoodLabel = lv_label_create(screen);
  lv_obj_set_style_text_color(robotMoodLabel, lvglColor565(C_UI_ACCENT), 0);
  lv_obj_set_style_text_font(robotMoodLabel, &lv_font_montserrat_20, 0);
  configureOpaqueLabel(robotMoodLabel, 340, 34);
  lv_label_set_text(robotMoodLabel, "READY TO RIDE");
  lv_obj_align(robotMoodLabel, LV_ALIGN_TOP_MID, 0, 386);
  lv_obj_add_flag(robotMoodLabel, LV_OBJ_FLAG_HIDDEN);

  stockTitleLabel = lv_label_create(screen);
  lv_obj_set_style_text_color(stockTitleLabel, lv_color_white(), 0);
  lv_obj_set_style_text_font(stockTitleLabel, &lv_font_montserrat_28, 0);
  lv_label_set_text(stockTitleLabel, "MARKET | DAY");
  lv_obj_align(stockTitleLabel, LV_ALIGN_TOP_MID, 0, 52);
  lv_obj_add_flag(stockTitleLabel, LV_OBJ_FLAG_HIDDEN);

  stockStatusLabel = lv_label_create(screen);
  lv_obj_set_style_text_color(stockStatusLabel, lvglColor565(C_UI_SUB), 0);
  lv_obj_set_style_text_font(stockStatusLabel, &lv_font_montserrat_16, 0);
  configureOpaqueLabel(stockStatusLabel, 360, 24);
  lv_label_set_text(stockStatusLabel, "Waiting for iPhone quotes");
  lv_obj_align(stockStatusLabel, LV_ALIGN_TOP_MID, 0, 88);
  lv_obj_add_flag(stockStatusLabel, LV_OBJ_FLAG_HIDDEN);

  stockList = lv_obj_create(screen);
  lv_obj_set_size(stockList, 344, 290);
  lv_obj_set_pos(stockList, CX - 172, 115);
  lv_obj_set_style_pad_all(stockList, 0, 0);
  lv_obj_set_style_border_width(stockList, 0, 0);
  lv_obj_set_style_bg_color(stockList, lv_color_black(), 0);
  lv_obj_set_scroll_dir(stockList, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(stockList, LV_SCROLLBAR_MODE_AUTO);
  lv_obj_add_flag(stockList, LV_OBJ_FLAG_SCROLL_MOMENTUM);
  lv_obj_add_flag(stockList, LV_OBJ_FLAG_HIDDEN);
  for (int i = 0; i < STOCK_MAX; i++) {
    lv_obj_t *row = lv_obj_create(stockList);
    lv_obj_set_size(row, 332, STOCK_ROW_H);
    lv_obj_set_pos(row, 0, i * (STOCK_ROW_H + 8));
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_style_radius(row, 8, 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_border_color(row, lvglColor565(C_UI_LINE), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(row, lv_color_hex(STOCK_PANEL_COLOR), 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);
    stockRows[i] = row;

    stockCodeLabels[i] = lv_label_create(row);
    lv_obj_add_flag(stockCodeLabels[i], LV_OBJ_FLAG_HIDDEN);

    stockNameBuffers[i] = (lv_color_t *)ps_malloc(
      STOCK_NAME_W * STOCK_NAME_H * sizeof(lv_color_t));
    if (stockNameBuffers[i]) {
      stockNameCanvases[i] = lv_canvas_create(row);
      lv_canvas_set_buffer(stockNameCanvases[i], stockNameBuffers[i],
                           STOCK_NAME_W, STOCK_NAME_H, LV_IMG_CF_TRUE_COLOR);
      lv_obj_set_pos(stockNameCanvases[i], 10, 8);
      lv_obj_clear_flag(stockNameCanvases[i], LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_clear_flag(stockNameCanvases[i], LV_OBJ_FLAG_CLICKABLE);
      updateStockNameCanvas(i);
    }

    stockPriceLabels[i] = lv_label_create(row);
    lv_obj_set_size(stockPriceLabels[i], 120, 30);
    lv_label_set_long_mode(stockPriceLabels[i], LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(stockPriceLabels[i], LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_color(stockPriceLabels[i], lv_color_white(), 0);
    lv_obj_set_style_text_font(stockPriceLabels[i], &lv_font_montserrat_24, 0);
    lv_label_set_text(stockPriceLabels[i], "--");
    lv_obj_align(stockPriceLabels[i], LV_ALIGN_TOP_RIGHT, -12, 4);

    stockChartBuffers[i] = (lv_color_t *)ps_malloc(
      STOCK_CHART_W * STOCK_CHART_H * sizeof(lv_color_t));
    if (stockChartBuffers[i]) {
      stockChartCanvases[i] = lv_canvas_create(row);
      lv_canvas_set_buffer(stockChartCanvases[i], stockChartBuffers[i],
                           STOCK_CHART_W, STOCK_CHART_H, LV_IMG_CF_TRUE_COLOR);
      lv_obj_set_pos(stockChartCanvases[i], 13, 42);
      lv_obj_clear_flag(stockChartCanvases[i], LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_clear_flag(stockChartCanvases[i], LV_OBJ_FLAG_CLICKABLE);
      updateStockChartCanvas(i);
    }

    stockChangeLabels[i] = lv_label_create(row);
    lv_obj_set_size(stockChangeLabels[i], 150, 24);
    lv_label_set_long_mode(stockChangeLabels[i], LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(stockChangeLabels[i], LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_color(stockChangeLabels[i], lvglColor565(C_UI_SUB), 0);
    lv_obj_set_style_text_font(stockChangeLabels[i], &lv_font_montserrat_16, 0);
    lv_label_set_text(stockChangeLabels[i], "--");
    lv_obj_align(stockChangeLabels[i], LV_ALIGN_BOTTOM_RIGHT, -12, -4);
  }

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
  configureOpaqueLabel(batteryLabel, 130, 22);
  lv_obj_set_style_text_font(batteryLabel, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(batteryLabel, lv_color_white(), 0);
  lv_obj_set_pos(batteryLabel, CX - 65, 24);
  lvglLastPage = -1;
  lvglLastSliding = false;
  lvglUiReady = true;
}

// Only the fully LVGL 3/4/5 pages enable this path for now. All other page
// transitions still use the legacy compositor until real hardware QA.
static bool lvglSwipeBegin(const uint16_t *oldFrame, const uint16_t *newFrame) {
  if (!lvglUiReady) return false;
  return motonavLvglSlideBegin(lvglSwipeLayer, oldFrame, newFrame);
}
static bool lvglSwipePaint(int amount, bool horizontal, int direction) {
  if (!lvglSwipeLayer.active) return false;
  motonavLvglSlideMove(lvglSwipeLayer, amount, horizontal, direction);
  // Must draw while the main loop deliberately skips lvglHybridTick().
  lv_refr_now(nullptr);
  return true;
}
static void lvglSwipeFinish() {
  motonavLvglSlideEnd(lvglSwipeLayer);
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

static void lvglHybridTick() {
  if (!lvglUiReady) return;

  setLvglHidden(batteryLabel, pairOn || callOn || fxOn || (page == PAGE_HOME && inNav));
  char batteryText[24];
  if (batPct >= 0) snprintf(batteryText, sizeof(batteryText), "BAT %d%%", batPct);
  else snprintf(batteryText, sizeof(batteryText), "BAT --");
  setLvglLabelText(batteryLabel, batteryText);
  // Weather main page uses a shared LVGL view; QWeather bitmap detail pages
  // remain on the original Canvas until their image assets are ported.
  bool showWeather = page == PAGE_WEATHER && (!wxHave || wxDetail == 0) &&
                     !sliding && !pairOn && !callOn && !fxOn;
  motonavWeatherViewVisible(lvglWeatherView, showWeather);
  if (showWeather) {
    MotoNavWeatherModel weather;
    weather.available = wxHave;
    weather.connected = bleConnected && bleAuthed;
    weather.hourly = wxHourly;
    weather.icon = wxIcon;
    weather.temperature = wxTemp;
    weather.forecastCount = wxN;
    weather.aqi = wxExtra ? wxAQI : -1;
    weather.alertCount = wxExtra ? wxAlerts : 0;
    weather.updated = wxTime.c_str();
    for (int i = 0; i < 6; ++i) {
      weather.forecastHourOrWeek[i] = wxWk[i];
      weather.forecastIcon[i] = wxFi[i];
      weather.forecastHigh[i] = wxFh[i];
      weather.forecastLowOrRain[i] = wxFl[i];
    }
    motonavWeatherViewUpdate(lvglWeatherView, weather);
  }
  motonavMapCanvasVisible(lvglMapCanvas,
    lvglMapCanvasCurrentFrame && page == PAGE_HOME && inNav &&
    mapModeActive() && !sliding && !pairOn && !callOn && !fxOn && !staleShown);
  bool showMapHud = page == PAGE_HOME && inNav && mapModeActive() &&
                    !sliding && !pairOn && !callOn && !fxOn;
  motonavNavigationHudVisible(lvglMapHud, showMapHud);
  if (showMapHud) {
    const bool over = hasCam && camLimit > 0 && speedKmh > camLimit;
    String cameraRange = distText(camDist);
    motonavNavigationHudUpdate(lvglMapHud, speedKmh, hasCam, camLimit,
                               cameraRange.c_str(), over,
                               lvglColor565(mapHudColor565()), lvglColor565(C_RED));
  }
  bool showNativeCues = showMapHud && lvglMapCanvasCurrentFrame &&
                        lvglNativeCueSupported() && !cruiseOn;
  motonavNavigationCuesVisible(lvglTurnCues, showNativeCues);
  if (showNativeCues) {
    float angle = 0;
    const bool far = navDir != "S" && navDist > mapUiFarStageMeters();
    if (!far) (void)dirAngle(navDir, angle);
    const bool near = navDir != "S" && navDist <= mapUiNearMeters();
    const bool veryNear = navDir != "S" && navDist <= mapUiNearStageMeters();
    float progress = mapUiRingActive() ? mapUiRingShown() : tripFrac();
    const bool trip = !mapUiRingActive() && progress >= 0;
    const bool track = navDir != "S" && !trip;
    String distance = distText(navDist);
    motonavNavigationCuesUpdate(lvglTurnCues, motonavCueKind(navDir.c_str()),
      angle, distance.c_str(),
      near, progress, mapUiRingActive(), track, trip,
      lvglColor565(trip ? TRIP_DONE_C : veryNear ? C_ORANGE : C_YELLOW));
  }
  bool showMusic = page == PAGE_MUSIC && !sliding && !pairOn && !callOn && !fxOn;
  motonavMusicStatusVisible(lvglMusicView, showMusic);
  if (showMusic) {
    motonavMusicStatusUpdate(lvglMusicView, coverValid && coverImg,
                             musicLyricsShownLVGL, musicStatusLVGL.c_str(),
                             musicFooterLVGL.c_str(), musicAck == 4);
  }
  const bool linkedMusic = showMusic && bleConnected && bleAuthed &&
                           !(hidMusicMode && !appleMedia.available);
  const bool showCover = showMusic && coverValid && coverImg;
  const bool showWeatherDetail = page == PAGE_WEATHER && wxHave && wxExtra &&
                                 wxDetail != 0 && !sliding && !pairOn && !callOn && !fxOn;
  for (int i = 0; i < 10; ++i) {
    const int slot = lvglTextSlots[i];
    const bool use = slot == 3 || slot == 4 ? showWeather && wxHave
      : slot == 5 || slot == 6 ? linkedMusic
      : slot == 12 ? linkedMusic && musicLyricsShownLVGL
      : slot == 7 || slot == 8 ? showWeatherDetail && wxDetail == 1
      : showWeatherDetail && wxDetail == 2;
    if (use) {
      const TextImg &source = imgs[slot];
      const int y = slot == 3 ? 56 : slot == 4 ? 232
        : slot == 5 ? (showCover ? 194 : 158)
        : slot == 6 ? (showCover ? 240 : 210)
        : slot == 7 ? 105 : slot == 8 ? 285
        : slot == 9 ? 125 : slot == 10 ? 200
        : slot == 11 ? 270 : 242;
      const uint8_t tint = slot == 4 ? 0xC0 : slot == 6 ? 0xBB
        : slot == 10 || slot == 11 ? 0xDC
        : slot == 12 ? 0xDD : 0xFF;
      lvglTextAssetReady[slot] = motonavRasterSetText(lvglTextAssets[i],
        source.show, IMG_MAX, source.showW, source.showH, y, tint,
        textImageRevision[slot]);
    }
    motonavRasterSetHidden(lvglTextAssets[i].image,
                           !use || !lvglTextAssets[i].ready);
  }
  if (showCover) {
    lvglCoverAssetReady = motonavCoverSet(lvglAlbumCover, coverImg,
                                           coverW, coverH, coverImageRevision);
  } else if (!coverValid) {
    lvglCoverAssetReady = false;
  }
  motonavCoverVisible(lvglAlbumCover, showCover);
  bool showSettings = page == 3 && !sliding && !pairOn && !callOn && !fxOn;
  bool showRobot = page == 4 && !sliding && !pairOn && !callOn && !fxOn;
  bool showStock = page == 5 && !sliding && !pairOn && !callOn && !fxOn;
  bool showHome = page == 0 && !inNav && !sliding && !pairOn && !callOn && !fxOn && !staleShown;
  motonavHomeChromeVisible(lvglHomeChrome, showHome);
  bool infoDots = page >= 3 && page <= 5;
  bool showDots = false;
  int buttonY = (coverValid && coverImg) ? 360 : 335;
  const int dotOrder[] = {1, 0, 2};

  for (int i = 0; i < 3; i++) {
    int x = CX + (i - 1) * 100 - 43;
    setLvglPos(musicButtons[i], x, buttonY - 43);
    setLvglHidden(musicButtons[i], !showMusic);
    bool enabled = hidMusicMode ? (hidMediaReady() || appleMedia.controlReady(i)) : (bleConnected && bleAuthed && musicHave);
    setLvglBgColor(musicButtons[i], lvglColor565(enabled ? C_UI_PANEL_2 : C_UI_PANEL));
    const char *symbol = i == 0 ? LV_SYMBOL_PREV : i == 2 ? LV_SYMBOL_NEXT
      : (musicPlaying ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
    setLvglLabelText(musicButtonLabels[i], symbol);
    setLvglTextColor(musicButtonLabels[i], lvglColor565(enabled ? C_UI_TEXT : C_UI_SUB));
  }

  setLvglHidden(settingsSlider, !showSettings);
  setLvglHidden(settingsBrightnessLabel, !showSettings);
  setLvglHidden(settingsTitleLabel, !showSettings);
  setLvglHidden(settingsSubLabel, !showSettings);
  setLvglHidden(settingsHintLabel, !showSettings);
  setLvglHidden(settingsCard, !showSettings);
  if (showSettings && !lv_obj_has_state(settingsSlider, LV_STATE_PRESSED)) {
    if (lv_slider_get_value(settingsSlider) != screenBrightness) {
      lv_slider_set_value(settingsSlider, screenBrightness, LV_ANIM_OFF);
    }
    char brightness[16];
    snprintf(brightness, sizeof(brightness), "%d%%", (screenBrightness * 100 + 92) / 180);
    setLvglLabelText(settingsBrightnessLabel, brightness);
  }

  for (int i = 0; i < 2; ++i) setLvglHidden(homeActions[i], !showHome);
  setLvglHidden(homeTitleLabel, !showHome);
  setLvglHidden(homeStatusDot, !showHome);
  setLvglHidden(homeStatusLabel, !showHome);
  setLvglHidden(homeDeviceLabel, true);
  setLvglHidden(homeHintLabel, true);
  if (showHome) {
    const char *status = waitSt == WAIT_LOST ? "Link lost" : ((bleConnected && bleAuthed) ? "Ready" : "No link");
    uint32_t statusColor = waitSt == WAIT_LOST ? C_UI_ACCENT2
      : ((bleConnected && bleAuthed) ? C_UI_ACCENT : C_UI_SUB);
    if (setLvglLabelText(homeStatusLabel, status)) {
      lv_obj_align(homeStatusLabel, LV_ALIGN_TOP_MID, 14, 198);
    }
    setLvglTextColor(homeStatusLabel, lvglColor565(statusColor));
    setLvglBgColor(homeStatusDot, lvglColor565(statusColor));
  }

  setLvglHidden(watchTimeLabel, !showHome);
  setLvglHidden(watchDateLabel, !showHome);
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
        setLvglLabelText(watchTimeLabel, timeText);
        setLvglLabelText(watchDateLabel, dateText);
      }
    }
    char temperature[16];
    snprintf(temperature, sizeof(temperature), wxHave ? "%d°" : "--°", wxTemp);
    setLvglLabelText(watchTempLabel, temperature);
    char forecast[36];
    if (wxHave && !wxHourly && wxN > 0)
      snprintf(forecast, sizeof(forecast), "%s %d°   |   %s %d°",
               LV_SYMBOL_DOWN, wxFl[0], LV_SYMBOL_UP, wxFh[0]);
    else if (wxHave && wxHourly && wxN > 0)
      snprintf(forecast, sizeof(forecast), "NEXT HOUR  %d°", wxFh[0]);
    else snprintf(forecast, sizeof(forecast), "WEATHER FROM PHONE");
    setLvglLabelText(watchForecastLabel, forecast);
  }

  setLvglHidden(robotHalo, !showRobot);
  setLvglHidden(robotBody, !showRobot);
  setLvglHidden(robotTitleLabel, !showRobot);
  setLvglHidden(robotMoodLabel, !showRobot);
  if (showRobot) {
    unsigned long now = millis();
    if (now - robotLastAnimAt >= 100) {
      robotLastAnimAt = now;
      bool online = bleConnected && bleAuthed;
      bool reacting = (int32_t)(robotReactUntil - now) > 0;
      bool blink = (now % (online ? 4800UL : 6200UL)) < (online ? 170UL : 450UL);
      if (blink != robotLastBlink || online != robotLastOnline || reacting != robotLastReacting) {
        robotLastBlink = blink;
        robotLastOnline = online;
        int eyeHeight = blink ? 5 : reacting ? 58 : 50;
        int eyeY = blink ? 48 : reacting ? 21 : 25;
        for (int i = 0; i < 2; i++) {
          lv_obj_set_height(robotEyes[i], eyeHeight);
          lv_obj_set_y(robotEyes[i], eyeY);
          lv_obj_set_width(robotEyes[i], reacting ? 33 : 28);
          lv_obj_set_x(robotEyes[i], i == 0 ? reacting ? 33 : 37 : 95);
        }
      }
      String mood = reacting ? "HELLO!" : online ? "READY TO RIDE" : "WAITING FOR PHONE";
      if (mood != robotLastMood || reacting != robotLastReacting) {
        robotLastMood = mood;
        robotLastReacting = reacting;
        lv_label_set_text(robotMoodLabel, robotLastMood.c_str());
        lv_obj_set_style_text_color(robotMoodLabel,
          lvglColor565(reacting ? C_UI_ACCENT2 : online ? C_UI_ACCENT : C_UI_SUB), 0);
      }
    }
  } else {
    robotLastAnimAt = 0;
    robotLastBlink = false;
    robotLastOnline = false;
    robotLastReacting = false;
    robotLastMood = "";
  }

  setLvglHidden(stockList, !showStock);
  setLvglHidden(stockTitleLabel, !showStock);
  setLvglHidden(stockStatusLabel, !showStock);
  if (showStock) {
    setLvglLabelText(stockTitleLabel,
      stockChartPeriod == 'M' ? "MARKET | 1M" : "MARKET | DAY");
    bool refresh = stockRevision != stockLastRevision || stockDirty;
    if (refresh) {
      stockLastRevision = stockRevision;
      stockDirty = false;
      for (int i = 0; i < STOCK_MAX; i++) {
        bool hasQuote = i < stockCount;
        setLvglHidden(stockRows[i], !hasQuote);
        if (!hasQuote) continue;
        const StockQuote &q = stockQuotes[i];
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
    for (int i = 0; i < STOCK_MAX; i++) {
      setLvglHidden(stockRows[i], i >= stockCount);
    }
    unsigned long now = millis();
    if (now - stockLastStatusAt >= 5000) {
      stockLastStatusAt = now;
      String status;
      if (!stockCount) status = "Waiting for iPhone quotes";
      else if (!(bleConnected && bleAuthed)) status = "Phone offline • showing last quotes";
      else if (stockUpdatedAt && now - stockUpdatedAt > 180000UL) status = "Quotes stale • open MotoNav on iPhone";
      else {
        struct tm local {};
        if (clockLocalTime(local)) {
          char text[40];
          snprintf(text, sizeof(text), "From iPhone • updated %02d:%02d", local.tm_hour, local.tm_min);
          status = text;
        } else status = "Synced with iPhone";
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
    // A changing native map HUD must not force the continuous map feed
    // into the synchronous non-map display path on every speed update.
    if (!(page == PAGE_HOME && inNav && mapModeActive()))
      frameIsMap = false;
  }
}
