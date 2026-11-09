#pragma once
// MotoNav 2.1 first-boot factory QA. Only built into explicitly requested
// --qa firmware. It is deliberately never enabled by the production build.
// No BLE traffic, no FAT formatting, no permanent screen-rotation writes.
#ifndef MOTONAV_BOARD_QA
#define MOTONAV_BOARD_QA 0
#endif

#if MOTONAV_BOARD_QA
#include <esp_heap_caps.h>
#include <cstdio>
#include <cstring>

// Serial report is durable across USB reconnects, NVS persists only its
// compact latest summary (not the raw logs).
static uint32_t qaTotal = 0, qaPass = 0, qaFail = 0, qaManual = 0;
static uint32_t qaWorstFlushUs = 0;
static uint32_t qaFreePsramStart = 0, qaFreePsramEnd = 0;
static int qaTouchCount = 0;
static uint32_t qaRunCount = 0;

static void qaResult(const char *test, const char *result, unsigned long value) {
  ++qaTotal;
  if (!strcmp(result, "PASS")) ++qaPass;
  else if (!strcmp(result, "FAIL")) ++qaFail;
  else ++qaManual;
  Serial.printf("QA,%s,%s,%lu\n", test, result, value);
}

static void qaDrawPattern(uint16_t color, const char *title, int degrees,
                          lv_obj_t *titleLabel) {
  setScreenDeg(degrees, false);
  gfx->fillScreen(color);
  // Crosshair and corner fiducials expose axis reversal, LCD RGB wiring
  // and the circle/466->480 scaler. Pixels cannot confirm optical quality.
  const uint16_t accent = (color == 0xFFFF) ? 0x0000 : 0xFFFF;
  gfx->drawLine(0, 233, 465, 233, accent);
  gfx->drawLine(233, 0, 233, 465, accent);
  gfx->drawCircle(233, 233, 205, accent);
  gfx->fillRect(32, 33, 38, 38, accent);
  gfx->fillTriangle(397, 33, 435, 71, 397, 71, accent);
  if (titleLabel) {
    lv_label_set_text(titleLabel, title);
    lv_obj_invalidate(titleLabel);
    lv_refr_now(nullptr);
  }
  const uint32_t t = micros();
  flushSync();
  const uint32_t elapsed = micros() - t;
  if (elapsed > qaWorstFlushUs) qaWorstFlushUs = elapsed;
  Serial.printf("QA,DISPLAY_PATTERN,OBSERVE,deg=%d,title=%s,flush_us=%lu\n",
                degrees, title, (unsigned long)elapsed);
}

static bool qaProbePsram() {
  constexpr size_t kProbeBytes = 16384;
  uint8_t *probe = static_cast<uint8_t *>(
      heap_caps_malloc(kProbeBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (!probe) return false;
  for (size_t i = 0; i < kProbeBytes; ++i)
    probe[i] = static_cast<uint8_t>((i * 37 + 11) & 255);
  bool ok = true;
  for (size_t i = 0; i < kProbeBytes; ++i)
    if (probe[i] != static_cast<uint8_t>((i * 37 + 11) & 255)) {
      ok = false; break;
    }
  heap_caps_free(probe);
  return ok;
}

static void qaWait(uint32_t milliseconds) {
  const uint32_t start = millis();
  while (millis() - start < milliseconds) delay(12);
}

static int qaTouchTargets(lv_obj_t *label) {
  // Touch is an interactive observation; a missing touch is NOT automatically
  // classified as hardware failure. Run for 6 seconds maximum.
  setScreenDeg(0, false);
  gfx->fillScreen(0);
  const int x[4] = {112, 350, 112, 350};
  const int y[4] = {112, 112, 350, 350};
  for (int k = 0; k < 4; ++k)
    gfx->fillCircle(x[k], y[k], 19, 0xFDE0);
  lv_label_set_text(label, "TOUCH THE 4 DOTS");
  lv_obj_invalidate(label);
  lv_refr_now(nullptr);
  flushSync();
  uint8_t seen = 0;
  bool lastDown = false;
  const uint32_t start = millis();
  while (millis() - start < 6500 && seen != 15) {
    touchPoll();
    if (touchDown && !lastDown) {
      const int tx = touchX1, ty = touchY1;
      Serial.printf("QA,TOUCH_SAMPLE,OBSERVE,x=%d,y=%d\n", tx, ty);
      for (int k = 0; k < 4; ++k) {
        const int dx = tx - x[k], dy = ty - y[k];
        if (dx * dx + dy * dy <= 55 * 55 && !(seen & (1 << k))) {
          seen |= uint8_t(1 << k);
          gfx->fillCircle(x[k], y[k], 19, 0x07E0);
          lv_obj_invalidate(label);
          lv_refr_now(nullptr);
          flushSync();
        }
      }
    }
    lastDown = touchDown;
    delay(20);
  }
  int count = 0;
  for (int k = 0; k < 4; ++k) if (seen & (1 << k)) ++count;
  return count;
}

static void qaSaveSummary() {
  char summary[370];
  snprintf(summary, sizeof(summary),
      "{\"schema\":1,\"firmware\":\"%s\",\"run\":%lu,"
      "\"pass\":%lu,\"fail\":%lu,\"manual\":%lu,\"total\":%lu,"
      "\"touch_dots\":%d,\"flush_worst_us\":%lu,"
      "\"psram_start\":%lu,\"psram_end\":%lu,"
      "\"optical_verified\":false}",
      FW_VERSION, (unsigned long)qaRunCount,
      (unsigned long)qaPass, (unsigned long)qaFail,
      (unsigned long)qaManual, (unsigned long)qaTotal,
      qaTouchCount, (unsigned long)qaWorstFlushUs,
      (unsigned long)qaFreePsramStart, (unsigned long)qaFreePsramEnd);
  Serial.printf("QA_REPORT,%s\n", summary);
  Preferences pref;
  if (pref.begin("motonavqa", false)) {
    pref.putString("last", summary);
    pref.end();
  } else Serial.println("QA,REPORT_SAVE,FAIL,0");
}

static void qaPrintSaved() {
  Preferences pref;
  String report;
  if (pref.begin("motonavqa", true)) {
    report = pref.getString("last", "");
    pref.end();
  }
  Serial.printf("QA_REPORT,%s\n", report.length() ? report.c_str() : "{}");
}

static void qaRunBoard() {
  // Must run on Arduino's main task, before BLE starts or while disconnected.
  ++qaRunCount;
  qaPass = qaFail = qaManual = qaTotal = 0;
  qaWorstFlushUs = 0;
  qaTouchCount = 0;
  qaFreePsramStart = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  const int originalRotation = screenDeg;
  Serial.printf("QA_BEGIN,version=%s,run=%lu,psram=%lu\n", FW_VERSION,
                (unsigned long)qaRunCount, (unsigned long)qaFreePsramStart);

  qaResult("PSRAM_PROBE", qaProbePsram() ? "PASS" : "FAIL",
           (unsigned long)qaFreePsramStart);
  const uint16_t *logical = gfx->getFramebuffer();
  qaResult("FRAMEBUFFER", logical ? "PASS" : "FAIL",
           logical ? LCD_WIDTH * LCD_HEIGHT * 2UL : 0);
  qaResult("RGB_DOUBLE_BUFFER", fbBuf[1] && flushReq && flushDone ? "PASS" : "FAIL",
           fbBuf[1] ? 1 : 0);
  qaResult("LVGL_INITIALIZED", lvglUiReady && lv_disp_get_default() ? "PASS" : "FAIL",
           lvglUiReady ? 1 : 0);
  qaResult("SLIDE_SNAPSHOTS", slideOld && slideNew ? "PASS" : "FAIL",
           slideOld && slideNew ? 2 : 0);

  if (!logical || !lvglUiReady) {
    qaFreePsramEnd = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    qaSaveSummary();
    Serial.println("QA_END,UNSAFE_TO_DRAW");
    return;
  }

  // The top layer is transient and never changes the real home screen tree.
  lv_obj_t *layer = lv_obj_create(lv_layer_top());
  lv_obj_remove_style_all(layer);
  lv_obj_set_size(layer, LCD_WIDTH, LCD_HEIGHT);
  lv_obj_set_pos(layer, 0, 0);
  lv_obj_set_style_bg_opa(layer, LV_OPA_TRANSP, 0);
  lv_obj_clear_flag(layer, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *label = lv_label_create(layer);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(label, lv_color_white(), 0);
  lv_obj_set_style_bg_color(label, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(label, LV_OPA_COVER, 0);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_width(label, LCD_WIDTH - 90);
  lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

  const uint16_t patterns[] = {0xF800, 0x07E0, 0x001F, 0xFFFF, 0x0000};
  const char *names[] = {"RED 0deg", "GREEN 90deg", "BLUE 314deg",
                         "WHITE 0deg", "BLACK 0deg"};
  const int degrees[] = {0, 90, 314, 0, 0};
  bool memoryPatterns = true;
  for (unsigned i = 0; i < 5; ++i) {
    gfx->fillScreen(patterns[i]);
    const uint16_t *p = gfx->getFramebuffer();
    for (int n = 0; n < 9; ++n) {
      int xx = 29 + 40 * n, yy = 35 + 30 * n;
      if (p[yy * LCD_WIDTH + xx] != patterns[i])
        memoryPatterns = false;
    }
    qaDrawPattern(patterns[i], names[i], degrees[i], label);
    qaWait(650);
  }
  qaResult("RGB565_RAM_READBACK", memoryPatterns ? "PASS" : "FAIL", 5);
  qaResult("VSYNC_FLUSH_TIMING",
           qaWorstFlushUs > 0 && qaWorstFlushUs < 500000 ? "PASS" : "FAIL",
           (unsigned long)qaWorstFlushUs);

  // Native LVGL geometry path, with a dedicated image source.
  const bool startedMap = lvglMapCanvasStartFrame();
  bool geometry = false;
  if (startedMap && lvglMapCanvasPixels()) {
    uint16_t *pixels = lvglMapCanvasPixels();
    for (int y = 0; y < LCD_HEIGHT; ++y)
      for (int x = 0; x < LCD_WIDTH; ++x)
        pixels[y * LCD_WIDTH + x] = ((x / 28 + y / 25) & 1) ? 0x39E7 : 0x18C3;
    const float ax[] = {65.f, 179.f, 165.f, 60.f};
    const float ay[] = {195.f, 179.f, 290.f, 285.f};
    const float lx[] = {90.f, 140.f, 235.f, 310.f, 370.f};
    const float ly[] = {380.f, 315.f, 305.f, 210.f, 190.f};
    geometry = lvglMapCanvasPolygon(ax, ay, 4, 0x07E0) &&
               lvglMapCanvasLine(lx, ly, 5, 14.f, 0xF800, true);
    lvglMapCanvasEndFrame();
    motonavMapCanvasVisible(lvglMapCanvas, true);
    lv_label_set_text(label, "LVGL MAP / ROUTE");
    lv_obj_invalidate(label);
    lv_refr_now(nullptr);
    flushSync();
    qaWait(900);
    motonavMapCanvasVisible(lvglMapCanvas, false);
    lvglMapCanvasFallback();
  }
  qaResult("LVGL_MAP_CANVAS", geometry ? "PASS" : "FAIL",
           (unsigned long)lvglMapCanvas.nativePolygons);

  qaTouchCount = qaTouchTargets(label);
  qaResult("TOUCH_FOUR_DOTS", qaTouchCount == 4 ? "PASS" : "MANUAL",
           (unsigned long)qaTouchCount);
  qaResult("OPTICAL_COLOR_ROTATION", "MANUAL", 3);

  setScreenDeg(originalRotation, false);
  qaFreePsramEnd = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  const bool retained = qaFreePsramEnd > 256 * 1024;
  qaResult("PSRAM_REMAINING", retained ? "PASS" : "FAIL",
           (unsigned long)qaFreePsramEnd);
  qaSaveSummary();
  gfx->fillScreen(0);
  lv_label_set_text_fmt(label, "QA: %lu PASS / %lu FAIL\n%lu NEED REVIEW",
                        (unsigned long)qaPass, (unsigned long)qaFail,
                        (unsigned long)qaManual);
  lv_obj_invalidate(label);
  lv_refr_now(nullptr);
  flushSync();
  qaWait(1800);
  lv_obj_del(layer);
  // Invalidate LVGL contents, refresh the regular home frame.
  gfx->fillScreen(0);
  drawWaiting(WAIT_NOLINK);
  lvglCanvasRepainted = true;
  lvglHybridTick();
  flushSync();
  needFlush = false;
  forceFullFlush = true;
  Serial.println("QA_END,NORMAL_BOOT");
}

static void qaSerialTick() {
  static char command[18] = {};
  static unsigned used = 0;
  while (Serial.available()) {
    const int c = Serial.read();
    if (c == '\r' || c == '\n') {
      command[used] = 0;
      if (!strcmp(command, "QA?") || !strcmp(command, "QAREPORT")) {
        qaPrintSaved();
      } else if (!strcmp(command, "QARUN")) {
        if (bleConnected || inNav || callOn || pairOn || sliding || fxOn ||
            touchDown) Serial.println("QA_REJECT,BUSY");
        else qaRunBoard();
      }
      used = 0;
    } else if (c >= 32 && c < 127) {
      if (used < sizeof(command) - 1) command[used++] = char(c);
      else used = 0;
    }
  }
}
#else
static inline void qaRunBoard() {}
static inline void qaSerialTick() {}
#endif
