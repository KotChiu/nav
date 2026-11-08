#pragma once

#include <lvgl.h>

static inline lv_color_t lvglColor565(uint16_t color) {
  lv_color_t out;
  out.full = color;
  return out;
}

static lv_disp_draw_buf_t lvglDrawBuf;
static lv_disp_drv_t lvglDispDrv;
static volatile bool lvglOverlayDirty = false;
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
static lv_obj_t *homeTitleLabel = nullptr;
static lv_obj_t *homeStatusLabel = nullptr;
static lv_obj_t *homeDeviceLabel = nullptr;
static lv_obj_t *homeHintLabel = nullptr;
static lv_obj_t *homeStatusDot = nullptr;
static bool dotsWereShown = false;
static lv_obj_t *watchTimeLabel = nullptr;
static lv_obj_t *watchDateLabel = nullptr;
static lv_obj_t *watchSecondsLabel = nullptr;
static lv_obj_t *watchHintLabel = nullptr;
static uint32_t lastWatchEpoch = 0;
static lv_obj_t *robotHalo = nullptr;
static lv_obj_t *robotBody = nullptr;
static lv_obj_t *robotFace = nullptr;
static lv_obj_t *robotEyes[2] = {nullptr, nullptr};
static lv_obj_t *robotMouth = nullptr;
static lv_obj_t *robotTitleLabel = nullptr;
static lv_obj_t *robotMoodLabel = nullptr;
static unsigned long robotLastAnimAt = 0;
static unsigned long robotReactUntil = 0;
static bool robotLastBlink = false;
static bool robotLastOnline = false;
static bool robotLastReacting = false;
static String robotLastMood;
static lv_obj_t *stockTitleLabel = nullptr;
static lv_obj_t *stockStatusLabel = nullptr;
static lv_obj_t *stockRows[4] = {nullptr, nullptr, nullptr, nullptr};
static lv_obj_t *stockCodeLabels[4] = {nullptr, nullptr, nullptr, nullptr};
static lv_obj_t *stockPriceLabels[4] = {nullptr, nullptr, nullptr, nullptr};
static lv_obj_t *stockChangeLabels[4] = {nullptr, nullptr, nullptr, nullptr};
static uint32_t stockLastRevision = UINT32_MAX;
static unsigned long stockLastStatusAt = 0;
static String stockLastStatus;
static lv_obj_t *transitionOldImg = nullptr;
static lv_obj_t *transitionNewImg = nullptr;
static lv_img_dsc_t transitionOldDsc;
static lv_img_dsc_t transitionNewDsc;
static bool transitionActive = false;
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
  data->state = touchDown ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
  if (touchDown) {
    data->point.x = touchX1;
    data->point.y = touchY1;
  }
}

static void musicButtonEventCb(lv_event_t *event) {
  if (lv_event_get_code(event) != LV_EVENT_PRESSED) return;
  int index = (int)(intptr_t)lv_event_get_user_data(event);
  int y = (coverValid && coverImg) ? 360 : 335;
  musicTap(CX + (index - 1) * 100, y);
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
  lv_obj_set_style_border_width(button, 0, 0);
  lv_obj_set_style_bg_color(button, lvglColor565(0x19A7), 0);
  lv_obj_set_style_bg_color(button, lvglColor565(0x0470), LV_STATE_PRESSED);
  lv_obj_set_style_shadow_width(button, 0, 0);
  lv_obj_clear_flag(button, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(button, musicButtonEventCb, LV_EVENT_PRESSED, (void *)(intptr_t)index);

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

static void clearCanvasRect(int x, int y, int w, int h) {
  uint16_t *fb = gfx->getFramebuffer();
  if (!fb) return;
  int x0 = max(0, x), x1 = min(LCD_WIDTH, x + w);
  for (int yy = max(0, y); yy < min(LCD_HEIGHT, y + h); yy++) {
    if (x1 > x0) memset(fb + (size_t)yy * LCD_WIDTH + x0, 0, (size_t)(x1 - x0) * 2);
  }
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

  settingsCard = lv_obj_create(screen);
  lv_obj_set_size(settingsCard, 340, 190);
  lv_obj_set_pos(settingsCard, CX - 170, 120);
  lv_obj_set_style_radius(settingsCard, 12, 0);
  lv_obj_set_style_border_width(settingsCard, 0, 0);
  lv_obj_set_style_bg_opa(settingsCard, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(settingsCard, lv_color_hex(0x101216), 0);
  lv_obj_clear_flag(settingsCard, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(settingsCard, LV_OBJ_FLAG_HIDDEN);

  settingsSlider = lv_slider_create(settingsCard);
  lv_obj_set_size(settingsSlider, 300, 28);
  lv_obj_set_pos(settingsSlider, 20, 84);
  lv_slider_set_range(settingsSlider, 5, 180);
  lv_slider_set_value(settingsSlider, screenBrightness, LV_ANIM_OFF);
  lv_obj_set_style_radius(settingsSlider, LV_RADIUS_CIRCLE, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(settingsSlider, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(settingsSlider, lv_color_hex(0x30343B), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(settingsSlider, LV_OPA_COVER, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(settingsSlider, lv_color_hex(0x41E7F2), LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(settingsSlider, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
  lv_obj_set_style_border_width(settingsSlider, 0, LV_PART_KNOB);
  lv_obj_set_style_pad_all(settingsSlider, 6, LV_PART_KNOB);
  lv_obj_add_event_cb(settingsSlider, brightnessEventCb, LV_EVENT_ALL, nullptr);
  lv_obj_add_flag(settingsSlider, LV_OBJ_FLAG_HIDDEN);

  settingsBrightnessLabel = lv_label_create(settingsCard);
  lv_obj_set_size(settingsBrightnessLabel, 92, 42);
  lv_obj_set_style_bg_opa(settingsBrightnessLabel, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(settingsBrightnessLabel, lv_color_hex(0x17191D), 0);
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
  lv_obj_set_style_text_color(settingsSubLabel, lv_color_hex(0x9AA3AD), 0);
  lv_obj_set_style_text_font(settingsSubLabel, &lv_font_montserrat_20, 0);
  lv_label_set_text(settingsSubLabel, "Display brightness");
  lv_obj_align(settingsSubLabel, LV_ALIGN_TOP_MID, 0, 132);
  lv_obj_add_flag(settingsSubLabel, LV_OBJ_FLAG_HIDDEN);

  settingsHintLabel = lv_label_create(screen);
  lv_obj_set_style_text_color(settingsHintLabel, lv_color_hex(0x69717A), 0);
  lv_obj_set_style_text_font(settingsHintLabel, &lv_font_montserrat_16, 0);
  lv_label_set_text(settingsHintLabel, "Swipe down to return");
  lv_obj_align(settingsHintLabel, LV_ALIGN_TOP_MID, 0, 400);
  lv_obj_add_flag(settingsHintLabel, LV_OBJ_FLAG_HIDDEN);

  homeTitleLabel = lv_label_create(screen);
  lv_obj_set_style_text_color(homeTitleLabel, lv_color_white(), 0);
  lv_obj_set_style_text_font(homeTitleLabel, &lv_font_montserrat_28, 0);
  lv_label_set_text(homeTitleLabel, "MotoNav");
  lv_obj_align(homeTitleLabel, LV_ALIGN_TOP_MID, 0, 128);
  lv_obj_add_flag(homeTitleLabel, LV_OBJ_FLAG_HIDDEN);

  homeStatusDot = lv_obj_create(screen);
  lv_obj_set_size(homeStatusDot, 12, 12);
  lv_obj_set_style_radius(homeStatusDot, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(homeStatusDot, 0, 0);
  lv_obj_clear_flag(homeStatusDot, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(homeStatusDot, LV_ALIGN_TOP_MID, -98, 218);
  lv_obj_add_flag(homeStatusDot, LV_OBJ_FLAG_HIDDEN);

  homeStatusLabel = lv_label_create(screen);
  lv_obj_set_style_text_font(homeStatusLabel, &lv_font_montserrat_20, 0);
  configureOpaqueLabel(homeStatusLabel, 220, 30);
  lv_obj_align(homeStatusLabel, LV_ALIGN_TOP_MID, 14, 214);
  lv_obj_add_flag(homeStatusLabel, LV_OBJ_FLAG_HIDDEN);

  homeDeviceLabel = lv_label_create(screen);
  lv_obj_set_style_text_color(homeDeviceLabel, lv_color_hex(0x9AA3AD), 0);
  lv_obj_set_style_text_font(homeDeviceLabel, &lv_font_montserrat_16, 0);
  configureOpaqueLabel(homeDeviceLabel, 300, 28);
  lv_obj_align(homeDeviceLabel, LV_ALIGN_TOP_MID, 0, 286);
  lv_obj_add_flag(homeDeviceLabel, LV_OBJ_FLAG_HIDDEN);

  homeHintLabel = lv_label_create(screen);
  lv_obj_set_style_text_color(homeHintLabel, lv_color_hex(0x69717A), 0);
  lv_obj_set_style_text_font(homeHintLabel, &lv_font_montserrat_16, 0);
  lv_label_set_text(homeHintLabel, "Swipe to explore");
  lv_obj_align(homeHintLabel, LV_ALIGN_TOP_MID, 0, 350);
  lv_obj_add_flag(homeHintLabel, LV_OBJ_FLAG_HIDDEN);

  watchTimeLabel = lv_label_create(screen);
  lv_obj_set_style_text_color(watchTimeLabel, lv_color_white(), 0);
  lv_obj_set_style_text_font(watchTimeLabel, &lv_font_montserrat_40, 0);
  configureOpaqueLabel(watchTimeLabel, 220, 64);
  lv_label_set_text(watchTimeLabel, "--:--");
  lv_obj_align(watchTimeLabel, LV_ALIGN_TOP_MID, 0, 136);
  lv_obj_add_flag(watchTimeLabel, LV_OBJ_FLAG_HIDDEN);

  watchDateLabel = lv_label_create(screen);
  lv_obj_set_style_text_color(watchDateLabel, lv_color_hex(0x9AA3AD), 0);
  lv_obj_set_style_text_font(watchDateLabel, &lv_font_montserrat_20, 0);
  configureOpaqueLabel(watchDateLabel, 330, 30);
  lv_label_set_text(watchDateLabel, "Waiting for phone time");
  lv_obj_align(watchDateLabel, LV_ALIGN_TOP_MID, 0, 218);
  lv_obj_add_flag(watchDateLabel, LV_OBJ_FLAG_HIDDEN);

  watchSecondsLabel = lv_label_create(screen);
  lv_obj_set_style_text_color(watchSecondsLabel, lv_color_hex(0x41E7F2), 0);
  lv_obj_set_style_text_font(watchSecondsLabel, &lv_font_montserrat_20, 0);
  configureOpaqueLabel(watchSecondsLabel, 80, 30);
  lv_label_set_text(watchSecondsLabel, "--");
  lv_obj_align(watchSecondsLabel, LV_ALIGN_TOP_MID, 0, 258);
  lv_obj_add_flag(watchSecondsLabel, LV_OBJ_FLAG_HIDDEN);

  watchHintLabel = lv_label_create(screen);
  lv_obj_set_style_text_color(watchHintLabel, lv_color_hex(0x69717A), 0);
  lv_obj_set_style_text_font(watchHintLabel, &lv_font_montserrat_16, 0);
  lv_label_set_text(watchHintLabel, "Swipe down to return");
  lv_obj_align(watchHintLabel, LV_ALIGN_TOP_MID, 0, 400);
  lv_obj_add_flag(watchHintLabel, LV_OBJ_FLAG_HIDDEN);

  robotHalo = lv_obj_create(screen);
  lv_obj_set_size(robotHalo, 258, 258);
  lv_obj_set_pos(robotHalo, CX - 129, 88);
  lv_obj_set_style_radius(robotHalo, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(robotHalo, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(robotHalo, 2, 0);
  lv_obj_set_style_border_color(robotHalo, lv_color_hex(0x18343A), 0);
  lv_obj_clear_flag(robotHalo, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(robotHalo, LV_OBJ_FLAG_HIDDEN);

  robotBody = lv_obj_create(screen);
  lv_obj_set_size(robotBody, 190, 170);
  lv_obj_set_pos(robotBody, CX - 95, 148);
  lv_obj_set_style_radius(robotBody, 58, 0);
  lv_obj_set_style_bg_opa(robotBody, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(robotBody, lv_color_hex(0xE9E6DC), 0);
  lv_obj_set_style_border_width(robotBody, 0, 0);
  lv_obj_set_style_shadow_width(robotBody, 0, 0);
  lv_obj_clear_flag(robotBody, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(robotBody, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(robotBody, robotTapEventCb, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_flag(robotBody, LV_OBJ_FLAG_HIDDEN);

  robotFace = lv_obj_create(robotBody);
  lv_obj_set_size(robotFace, 142, 80);
  lv_obj_align(robotFace, LV_ALIGN_TOP_MID, 0, 24);
  lv_obj_set_style_radius(robotFace, 34, 0);
  lv_obj_set_style_bg_opa(robotFace, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(robotFace, lv_color_hex(0x0C1216), 0);
  lv_obj_set_style_border_width(robotFace, 1, 0);
  lv_obj_set_style_border_color(robotFace, lv_color_hex(0x24505A), 0);
  lv_obj_clear_flag(robotFace, LV_OBJ_FLAG_SCROLLABLE);

  for (int i = 0; i < 2; i++) {
    robotEyes[i] = lv_obj_create(robotFace);
    lv_obj_set_size(robotEyes[i], 18, 26);
    lv_obj_set_pos(robotEyes[i], i == 0 ? 39 : 85, 24);
    lv_obj_set_style_radius(robotEyes[i], 9, 0);
    lv_obj_set_style_bg_opa(robotEyes[i], LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(robotEyes[i], lv_color_hex(0x41E7F2), 0);
    lv_obj_set_style_border_width(robotEyes[i], 0, 0);
    lv_obj_clear_flag(robotEyes[i], LV_OBJ_FLAG_SCROLLABLE);
  }

  robotMouth = lv_obj_create(robotFace);
  lv_obj_set_size(robotMouth, 28, 5);
  lv_obj_set_pos(robotMouth, 57, 60);
  lv_obj_set_style_radius(robotMouth, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(robotMouth, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(robotMouth, lv_color_hex(0x41E7F2), 0);
  lv_obj_set_style_border_width(robotMouth, 0, 0);
  lv_obj_clear_flag(robotMouth, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *chest = lv_obj_create(robotBody);
  lv_obj_set_size(chest, 34, 7);
  lv_obj_align(chest, LV_ALIGN_BOTTOM_MID, 0, -28);
  lv_obj_set_style_radius(chest, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(chest, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(chest, lv_color_hex(0xF5B84A), 0);
  lv_obj_set_style_border_width(chest, 0, 0);
  lv_obj_clear_flag(chest, LV_OBJ_FLAG_SCROLLABLE);

  robotTitleLabel = lv_label_create(screen);
  lv_obj_set_style_text_color(robotTitleLabel, lv_color_hex(0x7F8A93), 0);
  lv_obj_set_style_text_font(robotTitleLabel, &lv_font_montserrat_16, 0);
  lv_label_set_text(robotTitleLabel, "MOTONAV COMPANION");
  lv_obj_align(robotTitleLabel, LV_ALIGN_TOP_MID, 0, 54);
  lv_obj_add_flag(robotTitleLabel, LV_OBJ_FLAG_HIDDEN);

  robotMoodLabel = lv_label_create(screen);
  lv_obj_set_style_text_color(robotMoodLabel, lv_color_hex(0x41E7F2), 0);
  lv_obj_set_style_text_font(robotMoodLabel, &lv_font_montserrat_20, 0);
  configureOpaqueLabel(robotMoodLabel, 340, 34);
  lv_label_set_text(robotMoodLabel, "READY TO RIDE");
  lv_obj_align(robotMoodLabel, LV_ALIGN_TOP_MID, 0, 356);
  lv_obj_add_flag(robotMoodLabel, LV_OBJ_FLAG_HIDDEN);

  stockTitleLabel = lv_label_create(screen);
  lv_obj_set_style_text_color(stockTitleLabel, lv_color_white(), 0);
  lv_obj_set_style_text_font(stockTitleLabel, &lv_font_montserrat_28, 0);
  lv_label_set_text(stockTitleLabel, "MARKET WATCH");
  lv_obj_align(stockTitleLabel, LV_ALIGN_TOP_MID, 0, 52);
  lv_obj_add_flag(stockTitleLabel, LV_OBJ_FLAG_HIDDEN);

  stockStatusLabel = lv_label_create(screen);
  lv_obj_set_style_text_color(stockStatusLabel, lv_color_hex(0x7F8A93), 0);
  lv_obj_set_style_text_font(stockStatusLabel, &lv_font_montserrat_16, 0);
  configureOpaqueLabel(stockStatusLabel, 360, 24);
  lv_label_set_text(stockStatusLabel, "Waiting for iPhone quotes");
  lv_obj_align(stockStatusLabel, LV_ALIGN_TOP_MID, 0, 88);
  lv_obj_add_flag(stockStatusLabel, LV_OBJ_FLAG_HIDDEN);

  const int stockWidths[4] = {366, 352, 330, 292};
  for (int i = 0; i < 4; i++) {
    lv_obj_t *row = lv_obj_create(screen);
    lv_obj_set_size(row, stockWidths[i], 64);
    lv_obj_set_pos(row, CX - stockWidths[i] / 2, 114 + i * 72);
    lv_obj_set_style_radius(row, 12, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x10151A), 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);
    stockRows[i] = row;

    stockCodeLabels[i] = lv_label_create(row);
    lv_obj_set_style_text_color(stockCodeLabels[i], lv_color_white(), 0);
    lv_obj_set_style_text_font(stockCodeLabels[i], &lv_font_montserrat_20, 0);
    lv_label_set_text(stockCodeLabels[i], "---");
    lv_obj_set_pos(stockCodeLabels[i], 16, 7);

    stockPriceLabels[i] = lv_label_create(row);
    lv_obj_set_width(stockPriceLabels[i], 132);
    lv_obj_set_style_text_align(stockPriceLabels[i], LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_color(stockPriceLabels[i], lv_color_white(), 0);
    lv_obj_set_style_text_font(stockPriceLabels[i], &lv_font_montserrat_24, 0);
    lv_label_set_text(stockPriceLabels[i], "--");
    lv_obj_align(stockPriceLabels[i], LV_ALIGN_TOP_RIGHT, -16, 5);

    stockChangeLabels[i] = lv_label_create(row);
    lv_obj_set_width(stockChangeLabels[i], 104);
    lv_obj_set_style_text_align(stockChangeLabels[i], LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_color(stockChangeLabels[i], lv_color_hex(0x7F8A93), 0);
    lv_obj_set_style_text_font(stockChangeLabels[i], &lv_font_montserrat_16, 0);
    lv_label_set_text(stockChangeLabels[i], "--");
    lv_obj_align(stockChangeLabels[i], LV_ALIGN_BOTTOM_RIGHT, -16, -6);
  }

  transitionOldDsc.header.always_zero = 0;
  transitionOldDsc.header.cf = LV_IMG_CF_TRUE_COLOR;
  transitionOldDsc.header.w = LCD_WIDTH;
  transitionOldDsc.header.h = LCD_HEIGHT;
  transitionOldDsc.data_size = LCD_WIDTH * LCD_HEIGHT * sizeof(lv_color_t);
  transitionOldDsc.data = (const uint8_t *)slideOld;
  transitionNewDsc = transitionOldDsc;
  transitionNewDsc.data = (const uint8_t *)slideNew;

  transitionOldImg = lv_img_create(screen);
  lv_img_set_src(transitionOldImg, &transitionOldDsc);
  lv_obj_add_flag(transitionOldImg, LV_OBJ_FLAG_HIDDEN);
  transitionNewImg = lv_img_create(screen);
  lv_img_set_src(transitionNewImg, &transitionNewDsc);
  lv_obj_add_flag(transitionNewImg, LV_OBJ_FLAG_HIDDEN);
}

static void lvglTransitionBegin() {
  if (!lvglUiReady || !slideOld || !slideNew) return;
  transitionActive = true;
  transitionOldDsc.data = (const uint8_t *)slideOld;
  transitionNewDsc.data = (const uint8_t *)slideNew;
  lv_img_set_src(transitionOldImg, &transitionOldDsc);
  lv_img_set_src(transitionNewImg, &transitionNewDsc);
  lv_obj_t *screen = lv_scr_act();
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
  lv_obj_invalidate(transitionOldImg);
  lv_obj_invalidate(transitionNewImg);
  setLvglHidden(transitionOldImg, false);
  setLvglHidden(transitionNewImg, false);
  lvglTransitionUpdate(0);
}

static void lvglTransitionUpdate(int p) {
  if (!transitionActive) return;
  int full = dragHoriz ? LCD_WIDTH : LCD_HEIGHT;
  if (p < 0) p = 0;
  if (p > full) p = full;
  if (dragHoriz) {
    int oldX = dragVisualSign > 0 ? p : -p;
    int newX = dragVisualSign > 0 ? p - LCD_WIDTH : LCD_WIDTH - p;
    lv_obj_set_pos(transitionOldImg, oldX, 0);
    lv_obj_set_pos(transitionNewImg, newX, 0);
  } else {
    int oldY = dragVisualSign > 0 ? p : -p;
    int newY = dragVisualSign > 0 ? p - LCD_HEIGHT : LCD_HEIGHT - p;
    lv_obj_set_pos(transitionOldImg, 0, oldY);
    lv_obj_set_pos(transitionNewImg, 0, newY);
  }
}

static void lvglTransitionEnd() {
  if (!transitionActive) return;
  transitionActive = false;
  setLvglHidden(transitionOldImg, true);
  setLvglHidden(transitionNewImg, true);
  lv_obj_t *screen = lv_scr_act();
  lv_obj_set_style_bg_opa(screen, LV_OPA_TRANSP, 0);
  lv_obj_invalidate(screen);
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
  lv_indev_drv_register(&indevDrv);

  createLvglOverlay();
  lvglLastPage = -1;
  lvglLastSliding = false;
  lvglUiReady = true;
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

  bool showMusic = page == 2 && !sliding && !pairOn && !callOn && !fxOn;
  bool showSettings = page == 3 && !sliding && !pairOn && !callOn && !fxOn;
  bool showWatch = page == 4 && !sliding && !pairOn && !callOn && !fxOn;
  bool showRobot = page == 5 && !sliding && !pairOn && !callOn && !fxOn;
  bool showStock = page == 6 && !sliding && !pairOn && !callOn && !fxOn;
  bool showHome = page == 0 && !inNav && !sliding && !pairOn && !callOn && !fxOn && !staleShown;
  bool infoDots = page >= 3 && page <= 6;
  bool showDots = (page <= 2 || infoDots) && !sliding && !pairOn && !callOn && !fxOn && !(page == 0 && inNav);
  int buttonY = (coverValid && coverImg) ? 360 : 335;
  const int dotOrder[] = {1, 0, 2};

  for (int i = 0; i < 3; i++) {
    int x = CX + (i - 1) * 100 - 43;
    setLvglPos(musicButtons[i], x, buttonY - 43);
    setLvglHidden(musicButtons[i], !showMusic);
    bool enabled = hidMusicMode ? (hidMediaReady() || appleMedia.controlReady(i)) : (bleConnected && bleAuthed && musicHave);
    setLvglBgColor(musicButtons[i], lvglColor565(enabled ? 0x19A7 : 0x1082));
    const char *symbol = i == 0 ? LV_SYMBOL_PREV : i == 2 ? LV_SYMBOL_NEXT
      : (musicPlaying ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
    setLvglLabelText(musicButtonLabels[i], symbol);
    setLvglTextColor(musicButtonLabels[i], lvglColor565(enabled ? 0xFFFFFF : 0x7BEF));
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

  setLvglHidden(homeTitleLabel, !showHome);
  setLvglHidden(homeStatusDot, !showHome);
  setLvglHidden(homeStatusLabel, !showHome);
  setLvglHidden(homeDeviceLabel, !showHome);
  setLvglHidden(homeHintLabel, !showHome);
  if (showHome) {
    const char *status = waitSt == WAIT_LOST ? "Link lost" : ((bleConnected && bleAuthed) ? "Ready" : "No link");
    uint32_t statusColor = waitSt == WAIT_LOST ? 0xF800 : ((bleConnected && bleAuthed) ? 0x07E0 : 0x7BEF);
    if (setLvglLabelText(homeStatusLabel, status)) {
      lv_obj_align(homeStatusLabel, LV_ALIGN_TOP_MID, 14, 214);
    }
    setLvglTextColor(homeStatusLabel, lvglColor565(statusColor));
    if (setLvglLabelText(homeDeviceLabel, bleName.c_str())) {
      lv_obj_align(homeDeviceLabel, LV_ALIGN_TOP_MID, 0, 286);
    }
    setLvglBgColor(homeStatusDot, lvglColor565(statusColor));
  }

  setLvglHidden(watchTimeLabel, !showWatch);
  setLvglHidden(watchDateLabel, !showWatch);
  setLvglHidden(watchSecondsLabel, !showWatch);
  setLvglHidden(watchHintLabel, !showWatch);
  if (showWatch) {
    uint32_t epoch = clockNowEpoch();
    if (epoch && epoch != lastWatchEpoch) {
      lastWatchEpoch = epoch;
      struct tm local {};
      if (clockLocalTime(local)) {
        char timeText[16], secondText[12], dateText[40];
        static const char *const months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
          "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        snprintf(timeText, sizeof(timeText), "%02d:%02d", local.tm_hour, local.tm_min);
        snprintf(secondText, sizeof(secondText), "%02d", local.tm_sec);
        snprintf(dateText, sizeof(dateText), "%s %02d  %02d", months[local.tm_mon % 12],
                 local.tm_mday, local.tm_year + 1900);
        lv_label_set_text(watchTimeLabel, timeText);
        lv_label_set_text(watchSecondsLabel, secondText);
        lv_label_set_text(watchDateLabel, dateText);
      }
    }
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
      bool blink = !online || (now % 4800UL) < 170UL;
      if (blink != robotLastBlink || online != robotLastOnline) {
        robotLastBlink = blink;
        robotLastOnline = online;
        int eyeHeight = blink ? 4 : 26;
        int eyeY = blink ? 35 : 24;
        for (int i = 0; i < 2; i++) {
          lv_obj_set_height(robotEyes[i], eyeHeight);
          lv_obj_set_y(robotEyes[i], eyeY);
          lv_obj_set_style_bg_color(robotEyes[i],
            lv_color_hex(online ? 0x41E7F2 : 0x526169), 0);
        }
      }
      String mood = reacting ? "HELLO, RIDER" : online ? "READY TO RIDE" : "CONNECT YOUR PHONE";
      if (mood != robotLastMood || reacting != robotLastReacting) {
        robotLastMood = mood;
        robotLastReacting = reacting;
        lv_label_set_text(robotMoodLabel, robotLastMood.c_str());
        lv_obj_set_style_text_color(robotMoodLabel,
          lv_color_hex(reacting ? 0xF5B84A : online ? 0x41E7F2 : 0x7F8A93), 0);
        lv_obj_set_style_bg_color(robotMouth,
          lv_color_hex(reacting ? 0xF5B84A : online ? 0x41E7F2 : 0x526169), 0);
      }
    }
  } else {
    robotLastAnimAt = 0;
    robotLastBlink = false;
    robotLastOnline = false;
    robotLastReacting = false;
    robotLastMood = "";
  }

  setLvglHidden(stockTitleLabel, !showStock);
  setLvglHidden(stockStatusLabel, !showStock);
  if (showStock) {
    bool refresh = stockRevision != stockLastRevision;
    if (refresh) {
      stockLastRevision = stockRevision;
      for (int i = 0; i < 4; i++) {
        bool hasQuote = i < stockCount;
        setLvglHidden(stockRows[i], !hasQuote);
        if (!hasQuote) continue;
        const StockQuote &q = stockQuotes[i];
        char price[24], change[24];
        snprintf(price, sizeof(price), "%.2f", q.price);
        snprintf(change, sizeof(change), "%+.2f%%", q.changePct);
        lv_label_set_text(stockCodeLabels[i], q.code.c_str());
        lv_label_set_text(stockPriceLabels[i], price);
        lv_label_set_text(stockChangeLabels[i], change);
        uint32_t color = q.changePct > 0.0001f ? 0xFF5A68
          : q.changePct < -0.0001f ? 0x38D996 : 0x9AA3AD;
        lv_obj_set_style_text_color(stockChangeLabels[i], lvglColor565(color), 0);
      }
      stockLastStatusAt = 0;
    }
    for (int i = 0; i < 4; i++) {
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
    for (int i = 0; i < 4; i++) setLvglHidden(stockRows[i], true);
  }

  if (!showDots && dotsWereShown) {
    clearCanvasRect(CX - 44, 432, 88, 34);
  }
  dotsWereShown = showDots;

  int visibleDots = infoDots ? 4 : 3;
  int startX = infoDots ? CX - 27 : CX - 20;
  int spacing = infoDots ? 18 : 20;
  for (int i = 0; i < 4; i++) {
    setLvglPos(pageDotObjs[i], startX + i * spacing - 4, 440);
    setLvglHidden(pageDotObjs[i], !showDots || i >= visibleDots);
    bool active = infoDots ? page == 3 + i : i < 3 && page == dotOrder[i];
    setLvglBgColor(pageDotObjs[i], lvglColor565(active ? 0xFFFFFF : 0x4208));
  }

  if (page != lvglLastPage || sliding != lvglLastSliding) {
    lvglLastPage = page;
    lvglLastSliding = sliding;
  }

  lv_timer_handler();
  if (lvglOverlayDirty) {
    lvglOverlayDirty = false;
    needFlush = true;
    frameIsMap = false;
  }
}
