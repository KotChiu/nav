#pragma once
// Pure raster conversion used by LVGL on both ESP32-S3 and SDL2.
// Deliberately no Arduino, LVGL or device-specific includes.
#include <cmath>
#include <cstddef>
#include <cstdint>

struct MotoNavBitmapBox {
  int x = 0, y = 0, width = 0, height = 0;
  bool valid() const { return width > 0 && height > 0; }
};

// Reproduce the original circular text-safe-area scaling policy while also
// bounding the result to the 466x466 logical screen.
static inline MotoNavBitmapBox motonavBitmapBox(int width, int height, int y,
                                                int screen = 466, int radius = 200) {
  if (width <= 0 || height <= 0 || screen <= 0 || radius <= 0 ||
      width > 4096 || height > 4096) return {};
  const int center = screen / 2;
  float scale = 1.0f;
  for (int i = 0; i < 4; ++i) {
    const float scaledHeight = height * scale;
    const float dy = std::fmax(std::fabs(y - center),
                               std::fabs(y + scaledHeight - center));
    const float chord = dy < radius ?
      std::sqrt(float(radius * radius) - dy * dy) : 0.0f;
    if (width * scale / 2 <= chord) break;
    scale = 2.0f * chord / width;
  }
  // The previous Canvas path preserved a minimum 0.5x scale even if close to
  // the circular edge. Preserve that appearance but clip to screen bounds.
  if (scale < 0.5f) scale = 0.5f;
  if (width * scale > screen) scale = float(screen) / width;
  const int w = int(width * scale), h = int(height * scale);
  if (w <= 0 || h <= 0 || y >= screen || y + h <= 0) return {};
  return {center - w / 2, y, w, h};
}

static inline uint8_t motonavGrayNibble(const uint8_t *packed, int width,
                                        int x, int y) {
  const int bytesPerRow = (width + 1) / 2;
  const uint8_t byte = packed[size_t(y) * bytesPerRow + (x >> 1)];
  return (x & 1) ? (byte & 15) : (byte >> 4);
}

// Original area-averaged scaling (rather than sampling only one input pixel).
// Writes a 0..255 gray value; 0 is transparent in the LVGL image adapter.
template<class PutPixel>
static inline bool motonavRasterTextGray(const uint8_t *packed, size_t packedBytes,
                                         int width, int height, int outW, int outH,
                                         uint8_t tint, PutPixel put) {
  if (!packed || width <= 0 || height <= 0 || outW <= 0 || outH <= 0 ||
      width > 4096 || height > 4096 ||
      outW > 466 || outH > 466 ||
      size_t((width + 1) / 2) * size_t(height) > packedBytes) return false;
  for (int y = 0; y < outH; ++y) {
    const int sy0 = y * height / outH;
    const int sy1 = (y + 1) * height / outH > sy0 ?
      (y + 1) * height / outH : sy0 + 1;
    for (int x = 0; x < outW; ++x) {
      const int sx0 = x * width / outW;
      const int sx1 = (x + 1) * width / outW > sx0 ?
        (x + 1) * width / outW : sx0 + 1;
      unsigned sum = 0, count = 0;
      for (int sy = sy0; sy < sy1 && sy < height; ++sy)
        for (int sx = sx0; sx < sx1 && sx < width; ++sx) {
          sum += motonavGrayNibble(packed, width, sx, sy);
          ++count;
        }
      const uint8_t nibble = count ? uint8_t((sum + count / 2) / count) : 0;
      put(x, y, uint8_t((unsigned(nibble) * 17u * tint) / 255u));
    }
  }
  return true;
}

// CVR payload is decoded RGB565 by the existing BLE parser. Circular crop is
// applied here, without mutating or taking ownership of the source buffer.
template<class PutPixel>
static inline bool motonavRasterCover(const uint16_t *rgb565, int width, int height,
                                      int outputSize, PutPixel put) {
  if (!rgb565 || width <= 0 || height <= 0 || width > 128 || height > 128 ||
      outputSize <= 0 || outputSize > 128) return false;
  const float radius = outputSize / 2.0f - 0.5f;
  for (int y = 0; y < outputSize; ++y) {
    for (int x = 0; x < outputSize; ++x) {
      const float dx = x + 0.5f - outputSize / 2.0f;
      const float dy = y + 0.5f - outputSize / 2.0f;
      if (dx * dx + dy * dy > radius * radius) {
        put(x, y, uint16_t(0), false);
      } else {
        const int sx = x * width / outputSize;
        const int sy = y * height / outputSize;
        put(x, y, rgb565[size_t(sy) * width + sx], true);
      }
    }
  }
  return true;
}
