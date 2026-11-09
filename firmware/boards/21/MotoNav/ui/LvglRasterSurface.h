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
#include "Rgb565Plane.h"

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
    plane().pixel(x, y, color);
  }
  void writeFastHLine(int16_t x, int16_t y, int16_t len,
                      uint16_t color) override {
    plane().hline(x, y, len, color);
  }
  void writeFastVLine(int16_t x, int16_t y, int16_t len,
                      uint16_t color) override {
    plane().vline(x, y, len, color);
  }
  void writeFillRectPreclipped(int16_t x, int16_t y, int16_t w,
                                int16_t h, uint16_t color) override {
    plane().rect(x, y, w, h, color);
  }
  void fillScreen(uint16_t color) {
    plane().rect(0, 0, WIDTH, HEIGHT, color);
    lvglCanvasRepainted = true;
    forceFullFlush = true;
  }

 private:
  MotoNavRgb565Plane plane() {
    return MotoNavRgb565Plane{frame_, WIDTH, HEIGHT};
  }

  Arduino_G *output_;
  uint16_t *owned_ = nullptr;
  uint16_t *frame_ = nullptr;
};
