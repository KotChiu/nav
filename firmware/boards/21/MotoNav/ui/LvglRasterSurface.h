#pragma once
// Temporary RGB565 software drawing adapter used while legacy primitives
// are moved to native LVGL Canvas/image/objects. This deliberately subclasses
// Arduino_GFX (generic draw API), NOT Arduino_Canvas. The RGB hardware driver
// remains Board21Panel, and LVGL remains the UI compositor.
// All operations target 466x466 local pixels; final 480x480 rotation and DMA
// are handled separately by DisplayPipeline.h.
#include "Arduino_GFX_Library.h"
#include <esp_heap_caps.h>
#include <cstring>

class MotoNavRasterSurface : public Arduino_GFX {
 public:
  MotoNavRasterSurface(int16_t width, int16_t height, Arduino_G *output)
      : Arduino_GFX(width, height), output_(output) {}
  ~MotoNavRasterSurface() {
    if (owned_) heap_caps_free(owned_);
  }
  bool begin(int32_t speed = GFX_NOT_DEFINED) override {
    if (speed != GFX_SKIP_OUTPUT_BEGIN && output_ && !output_->begin(speed))
      return false;
    if (!owned_) {
      const size_t bytes = size_t(WIDTH) * size_t(HEIGHT) * sizeof(uint16_t);
      owned_ = static_cast<uint16_t *>(
          heap_caps_aligned_alloc(16, bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
      if (!owned_) return false;
      std::memset(owned_, 0, bytes);
      frame_ = owned_;
    }
    return true;
  }
  uint16_t *getFramebuffer() { return frame_; }
  void setFramebuffer(uint16_t *buffer) {
    // The caller owns the alternate frame (LVGL map source or page snapshot).
    frame_ = buffer;
  }
  void flush(bool forceFlush = false) override {
    (void)forceFlush;
    if (frame_) flushLogicalFrame(frame_);
  }

  void writePixelPreclipped(int16_t x, int16_t y, uint16_t color) override {
    if (frame_ && (unsigned)x < (unsigned)WIDTH &&
                  (unsigned)y < (unsigned)HEIGHT)
      frame_[size_t(y) * WIDTH + x] = color;
  }
  void writeFastHLine(int16_t x, int16_t y, int16_t len,
                      uint16_t color) override {
    if (!frame_ || y < 0 || y >= HEIGHT || len == 0) return;
    int a = x, b = x + len;
    if (len < 0) { a = x + len + 1; b = x + 1; }
    if (b <= 0 || a >= WIDTH) return;
    if (a < 0) a = 0;
    if (b > WIDTH) b = WIDTH;
    auto *row = frame_ + size_t(y) * WIDTH;
    for (int ix = a; ix < b; ++ix) row[ix] = color;
  }
  void writeFastVLine(int16_t x, int16_t y, int16_t len,
                      uint16_t color) override {
    if (!frame_ || x < 0 || x >= WIDTH || len == 0) return;
    int a = y, b = y + len;
    if (len < 0) { a = y + len + 1; b = y + 1; }
    if (b <= 0 || a >= HEIGHT) return;
    if (a < 0) a = 0;
    if (b > HEIGHT) b = HEIGHT;
    for (int iy = a; iy < b; ++iy)
      frame_[size_t(iy) * WIDTH + x] = color;
  }
  void writeFillRectPreclipped(int16_t x, int16_t y,
                                int16_t w, int16_t h,
                                uint16_t color) override {
    if (!frame_ || w <= 0 || h <= 0) return;
    const int left = x < 0 ? 0 : x;
    const int top = y < 0 ? 0 : y;
    const int right = x + w > WIDTH ? WIDTH : x + w;
    const int bottom = y + h > HEIGHT ? HEIGHT : y + h;
    if (left >= right || top >= bottom) return;
    for (int iy = top; iy < bottom; ++iy) {
      uint16_t *row = frame_ + size_t(iy) * WIDTH;
      for (int ix = left; ix < right; ++ix) row[ix] = color;
    }
  }

  void fillScreen(uint16_t color) {
    writeFillRectPreclipped(0, 0, WIDTH, HEIGHT, color);
    lvglCanvasRepainted = true;
    forceFullFlush = true;
  }

 private:
  Arduino_G *output_;
  uint16_t *owned_ = nullptr;
  uint16_t *frame_ = nullptr;
};
