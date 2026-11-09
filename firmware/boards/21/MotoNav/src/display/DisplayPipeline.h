#pragma once
#include "../../ui/PageSlideBlit.h"
// Double buffering and rotation for the Waveshare 2.1-inch RGB panel.
// Rendering remains at the 466x466 logical size; flush maps it into 480x480.

uint16_t *fbBuf[2] = {nullptr, nullptr};
int fbCur = 0;
SemaphoreHandle_t flushReq = nullptr, flushDone = nullptr;
uint16_t *volatile flushSrc = nullptr;
volatile uint32_t flushMsAcc = 0, flushCnt = 0;
volatile uint32_t flushVsyncUsAcc = 0, flushCopyUsAcc = 0, flushDrawUsAcc = 0;
volatile bool flushFullFrame = true;
volatile bool forceFullFlush = true;
volatile int flushRectCount = 0;
volatile FlushRect flushRects[FLUSH_RECT_MAX];

struct PanelSlideFrame {
  const uint16_t *oldFrame = nullptr;
  const uint16_t *newFrame = nullptr;
  MotoNavSlidePositions positions;
};
// Published with flushReq; the worker owns it until giving flushDone.
static PanelSlideFrame pendingPanelSlide;

int screenRot = 0;
int screenDeg = 0;
bool rotFree = false;
int32_t rotCos = 65536, rotSin = 0;
float rotCosF = 1.0f, rotSinF = 0.0f;
unsigned long rotSaveAt = 0;

void setScreenDeg(int d, bool save) {
  d = ((d % 360) + 360) % 360;
  screenDeg = d;
  rotFree = (d % 90) != 0;
  screenRot = rotFree ? 0 : d / 90;
  float a = d * (float)M_PI / 180.0f;
  rotCosF = cosf(a);
  rotSinF = sinf(a);
  rotCos = (int32_t)lroundf(rotCosF * 65536.0f);
  rotSin = (int32_t)lroundf(rotSinF * 65536.0f);
  if (save) rotSaveAt = millis() + 1500;
}

void initRotSpan() {}

void saveRotIfDue() {
  if (!rotSaveAt || (long)(millis() - rotSaveAt) < 0) return;
  rotSaveAt = 0;
  Preferences pr;
  pr.begin("motonav", false);
  pr.putInt("rotd", screenDeg);
  pr.putInt("rot", screenRot);
  pr.end();
}

void saveBrightnessIfDue() {
  if (!brightnessSaveAt || (long)(millis() - brightnessSaveAt) < 0) return;
  brightnessSaveAt = 0;
  Preferences pr;
  pr.begin("motonav", false);
  pr.putInt("bright", screenBrightness);
  pr.end();
}

bool fastFlushInit() { return false; }

void flushSetFullFrame() {
  if (flushDone) { flushWait(); xSemaphoreGive(flushDone); }
  flushFullFrame = true;
  flushRectCount = 0;
}

void flushSetDirtyRects(const FlushRect *rects, int count) {
  if (flushDone) { flushWait(); xSemaphoreGive(flushDone); }
  flushFullFrame = false;
  flushRectCount = constrain(count, 0, FLUSH_RECT_MAX);
  for (int i = 0; i < flushRectCount; i++) {
    flushRects[i].x1 = rects[i].x1;
    flushRects[i].y1 = rects[i].y1;
    flushRects[i].x2 = rects[i].x2;
    flushRects[i].y2 = rects[i].y2;
  }
}

static void flushPanelFrame(const uint16_t *fb, bool fullFrame,
                            const FlushRect *rects, int rectCount,
                            const PanelSlideFrame *slide) {
  if (!fb && !slide) return;

  const uint32_t phaseAt = micros();
  const bool frameSwap = board21Rgb->frameBoundarySwapActive();
  uint16_t *target = board21Rgb->acquireFrameBuffer(pdMS_TO_TICKS(250));
  const uint32_t readyAt = micros();
  if (!target) {
    if (frameSwap) {
      Serial.println("DISPLAY FATAL: scanout buffer unavailable");
      ESP.restart();
    }
    return;
  }

  // The inactive frame is two submissions old. Apply the previous dirty
  // rectangles as well as the current ones to keep both images equivalent.
  static FlushRect previousRects[FLUSH_RECT_MAX];
  static int previousCount = 0;
  static bool previousFull = true;
  static bool borderClean[2] = {false, false};
  const int targetIndex = frameSwap && target == board21Rgb->getFrameBuffer(1);
  const bool unrotated = slide || (screenRot == 0 && !rotFree);
  if (unrotated) {
    if (!borderClean[targetIndex]) {
      memset(target, 0, PANEL_WIDTH * PANEL_HEIGHT * 2);
      borderClean[targetIndex] = true;
    }
    const int ox = (PANEL_WIDTH - LCD_WIDTH) / 2;
    const int oy = (PANEL_HEIGHT - LCD_HEIGHT) / 2;
    bool copyFull = fullFrame || (frameSwap && previousFull);
    if (frameSwap && !copyFull) {
      auto area = [](const FlushRect *regions, int count) {
        int pixels = 0;
        for (int i = 0; i < count; ++i) {
          const FlushRect &r = regions[i];
          pixels += (r.x2 - r.x1 + 1) * (r.y2 - r.y1 + 1);
        }
        return pixels;
      };
      copyFull = area(previousRects, previousCount) + area(rects, rectCount) >=
                 LCD_WIDTH * LCD_HEIGHT;
    }
    if (slide) {
      motonavBlitPageSlide(target + (size_t)oy * PANEL_WIDTH + ox, PANEL_WIDTH,
                           slide->oldFrame, slide->newFrame, LCD_WIDTH,
                           LCD_HEIGHT, slide->positions);
    } else if (copyFull) {
      for (int y = 0; y < LCD_HEIGHT; y++) {
        memcpy(target + (size_t)(y + oy) * PANEL_WIDTH + ox,
               fb + (size_t)y * LCD_WIDTH, LCD_WIDTH * 2);
      }
    } else {
      auto copyRects = [&](const FlushRect *regions, int count) {
        for (int i = 0; i < count; ++i) {
          const FlushRect &r = regions[i];
          const int x1 = constrain(r.x1, 0, LCD_WIDTH - 1);
          const int width = constrain(r.x2, 0, LCD_WIDTH - 1) - x1 + 1;
          const int y1 = constrain(r.y1, 0, LCD_HEIGHT - 1);
          const int y2 = constrain(r.y2, 0, LCD_HEIGHT - 1);
          if (width <= 0) continue;
          for (int y = y1; y <= y2; ++y) {
            memcpy(target + (size_t)(y + oy) * PANEL_WIDTH + ox + x1,
                   fb + (size_t)y * LCD_WIDTH + x1, (size_t)width * 2);
          }
        }
      };
      if (frameSwap) copyRects(previousRects, previousCount);
      copyRects(rects, rectCount);
    }
  } else if (!rotFree) {
    // Quarter turns need only integer addressing, not bilinear resampling.
    const int border = (PANEL_WIDTH - LCD_WIDTH) / 2;
    memset(target, 0, PANEL_WIDTH * PANEL_HEIGHT * sizeof(uint16_t));
    for (int y = 0; y < LCD_HEIGHT; ++y) {
      uint16_t *row = target + (y + border) * PANEL_WIDTH + border;
      for (int x = 0; x < LCD_WIDTH; ++x) {
        int sx = screenRot == 1 ? y : screenRot == 2 ? LCD_WIDTH - 1 - x : LCD_WIDTH - 1 - y;
        int sy = screenRot == 1 ? LCD_HEIGHT - 1 - x : screenRot == 2 ? LCD_HEIGHT - 1 - y : x;
        row[x] = fb[sy * LCD_WIDTH + sx];
      }
    }
  } else {
    borderClean[targetIndex] = false;
    // Inverse affine mapping: compute a row origin once and advance by adds.
    // Nearest sampling avoids four PSRAM reads and bilinear math per pixel.
    const int32_t center = (LCD_WIDTH - 1) * 32768;
    const int32_t stepX = (int32_t)((int64_t)rotCos * LCD_WIDTH / PANEL_WIDTH);
    const int32_t stepY = (int32_t)(-(int64_t)rotSin * LCD_WIDTH / PANEL_WIDTH);
    const int32_t physicalX = portScale.axis[0] - center;
    const int sampleStep = 1;
    for (int y = 0; y < PANEL_HEIGHT; y += sampleStep) {
      const int32_t physicalY = portScale.axis[y] - center;
      int32_t fx = (int32_t)(((int64_t)physicalX * rotCos + (int64_t)physicalY * rotSin) >> 16) + center;
      int32_t fy = (int32_t)((-(int64_t)physicalX * rotSin + (int64_t)physicalY * rotCos) >> 16) + center;
      uint16_t *row = target + y * PANEL_WIDTH;
      for (int x = 0; x < PANEL_WIDTH; x += sampleStep, fx += stepX * sampleStep, fy += stepY * sampleStep) {
        int sx = (fx + 32768) >> 16, sy = (fy + 32768) >> 16;
        uint16_t pixel = (unsigned)sx < LCD_WIDTH && (unsigned)sy < LCD_HEIGHT
          ? fb[sy * LCD_WIDTH + sx] : 0;
        row[x] = pixel;
        if (sampleStep == 2) row[x + 1] = pixel;
      }
      if (sampleStep == 2) memcpy(row + PANEL_WIDTH, row, PANEL_WIDTH * sizeof(uint16_t));
    }
  }
  const uint32_t copyAt = micros();
  if (!frameSwap) board21Rgb->waitVsync();
  const uint32_t vsyncAt = micros();
  if (!board21Rgb->drawFrameBuffer(target, PANEL_WIDTH, PANEL_HEIGHT)) {
    Serial.println("DISPLAY FATAL: scanout submission failed");
    ESP.restart();
    return;
  }
  const uint32_t drawAt = micros();
  if (frameSwap) {
    previousFull = fullFrame || !unrotated;
    previousCount = previousFull ? 0 : rectCount;
    if (previousCount) memcpy(previousRects, rects, previousCount * sizeof(FlushRect));
  }
  const uint32_t waitUs = (readyAt - phaseAt) + (vsyncAt - copyAt);
  static uint8_t diagnosticFrames = 0;
  if (diagnosticFrames < 6) {
    diagnosticFrames++;
    Serial.printf("DISPLAY deg=%d full=%d copy=%luus vsync=%luus submit=%luus\n",
      screenDeg, fullFrame, (unsigned long)(copyAt-phaseAt),
      (unsigned long)(vsyncAt-copyAt), (unsigned long)(drawAt-vsyncAt));
  }
  flushCopyUsAcc += copyAt - readyAt;
  flushVsyncUsAcc += waitUs;
  flushDrawUsAcc += drawAt - vsyncAt;
}

void flushLogicalFrame(const uint16_t *fb, bool fullFrame,
                       const FlushRect *rects, int rectCount) {
  flushPanelFrame(fb, fullFrame, rects, rectCount, nullptr);
}

void flushLogicalFrame(const uint16_t *fb) {
  flushLogicalFrame(fb, true, nullptr, 0);
}

static bool panelSlideAvailable() {
  return flushDone && screenDeg == 0 && board21Rgb->frameBoundarySwapActive();
}

static bool tryFlushPanelSlide(const uint16_t *oldFrame, const uint16_t *newFrame,
                               int amount, bool horizontal, int direction) {
  if (xSemaphoreTake(flushDone, 0) != pdTRUE) return false;
  pendingPanelSlide = {oldFrame, newFrame, motonavSlidePositions(
      amount, horizontal, direction, LCD_WIDTH, LCD_HEIGHT)};
  flushFullFrame = true;
  flushRectCount = 0;
  xSemaphoreGive(flushReq);
  return true;
}

void flushTask(void *arg) {
  for (;;) {
    xSemaphoreTake(flushReq, portMAX_DELAY);
    uint32_t t0 = millis();
    if (pendingPanelSlide.oldFrame) {
      flushPanelFrame(nullptr, true, nullptr, 0, &pendingPanelSlide);
      pendingPanelSlide = {};
    } else {
      flushLogicalFrame(flushSrc, flushFullFrame, (const FlushRect *)flushRects,
                        flushRectCount);
    }
    flushMsAcc += millis() - t0;
    flushCnt++;
    xSemaphoreGive(flushDone);
    vTaskDelay(1);  // Keep IDLE0 alive during continuous map refresh.
  }
}

void flushWait() {
  if (flushDone && xSemaphoreTake(flushDone, pdMS_TO_TICKS(3000)) == pdTRUE)
    return;
  // Never reuse a source buffer while a stalled VSYNC/DMA task might still
  // read it. A bounded failure/reboot is safer than a permanently frozen
  // navigation display or corruption of an in-flight frame.
  Serial.println("DISPLAY FATAL: frame ownership/flush wait timed out");
  ESP.restart();
  abort();
}

void flushAsync() {
  if (lvglUiReady && fbBuf[1]) {
    // LVGL keeps its canonical buffer; transport owns an immutable snapshot.
    flushWait();
    memcpy(fbBuf[1], gfx->getFramebuffer(), LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t));
    flushSrc = fbBuf[1];
    flushFullFrame = true;
    flushRectCount = 0;
    xSemaphoreGive(flushReq);
    return;
  }
  if (!fbBuf[1]) {
    gfx->flush();
    return;
  }
  flushWait();
  flushSrc = fbBuf[fbCur];
  xSemaphoreGive(flushReq);
  fbCur ^= 1;
  gfx->setFramebuffer(fbBuf[fbCur]);
}

void flushSync() {
  if (!fbBuf[1]) {
    gfx->flush();
    return;
  }
  flushWait();
  flushSrc = fbBuf[fbCur];
  xSemaphoreGive(flushReq);
  flushWait();
  xSemaphoreGive(flushDone);
}

void initDoubleBuffer() {
  fbBuf[0] = gfx->getFramebuffer();
  const size_t bytes = (size_t)LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t);
  if (!fbBuf[0]) {
    Serial.println("DISPLAY: no logical framebuffer; use synchronous fallback");
    return;
  }
  fbBuf[1] = (uint16_t *)heap_caps_aligned_alloc(16, bytes, MALLOC_CAP_SPIRAM);
  if (!fbBuf[1]) {
    Serial.println("DISPLAY: transport PSRAM unavailable; using synchronous fallback");
    return;
  }
  memcpy(fbBuf[1], fbBuf[0], bytes);
  flushReq = xSemaphoreCreateBinary();
  flushDone = xSemaphoreCreateBinary();
  if (!flushReq || !flushDone ||
      xTaskCreatePinnedToCore(flushTask, "flush", 4096, nullptr, 1,
                              nullptr, 0) != pdPASS) {
    Serial.println("DISPLAY: flush task/semaphore failed; synchronous fallback");
    if (flushReq) { vSemaphoreDelete(flushReq); flushReq = nullptr; }
    if (flushDone) { vSemaphoreDelete(flushDone); flushDone = nullptr; }
    heap_caps_free(fbBuf[1]);
    fbBuf[1] = nullptr;
    return;
  }
  xSemaphoreGive(flushDone);
}
