#pragma once
// Host-testable 16-bit RGB565 framebuffer primitives used by the transitional
// Arduino_GFX-compatible renderer; no Arduino, LVGL or ESP headers.
#include <cstddef>
#include <cstdint>

struct MotoNavRgb565Plane {
  uint16_t *data = nullptr;
  int width = 0, height = 0;

  void pixel(int x, int y, uint16_t color) {
    if (data && x >= 0 && x < width && y >= 0 && y < height)
      data[static_cast<size_t>(y) * width + x] = color;
  }
  void hline(int x, int y, int len, uint16_t color) {
    if (!data || y < 0 || y >= height || !len) return;
    int left = x, right = x + len;
    if (len < 0) { left = x + len + 1; right = x + 1; }
    if (left < 0) left = 0;
    if (right > width) right = width;
    if (left >= right) return;
    auto *row = data + static_cast<size_t>(y) * width;
    for (int ix = left; ix < right; ++ix) row[ix] = color;
  }
  void vline(int x, int y, int len, uint16_t color) {
    if (!data || x < 0 || x >= width || !len) return;
    int top = y, bottom = y + len;
    if (len < 0) { top = y + len + 1; bottom = y + 1; }
    if (top < 0) top = 0;
    if (bottom > height) bottom = height;
    if (top >= bottom) return;
    for (int iy = top; iy < bottom; ++iy)
      data[static_cast<size_t>(iy) * width + x] = color;
  }
  void rect(int x, int y, int w, int h, uint16_t color) {
    if (!data || w <= 0 || h <= 0) return;
    int left = x < 0 ? 0 : x;
    int top = y < 0 ? 0 : y;
    int right = x + w > width ? width : x + w;
    int bottom = y + h > height ? height : y + h;
    if (left >= right || top >= bottom) return;
    for (int iy = top; iy < bottom; ++iy) {
      uint16_t *row = data + static_cast<size_t>(iy) * width;
      for (int ix = left; ix < right; ++ix) row[ix] = color;
    }
  }
};
