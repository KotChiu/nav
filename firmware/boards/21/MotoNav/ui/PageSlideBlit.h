#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include "PageSlideGeometry.h"

// Blit the same two opaque images as the LVGL transition, into a viewport
// whose rows may be wider than the logical snapshots.
static inline void motonavBlitPageSlide(
    uint16_t *target, size_t stride, const uint16_t *oldFrame,
    const uint16_t *newFrame, int width, int height,
    const MotoNavSlidePositions &positions) {
  for (int y = 0; y < height; ++y) {
    const int oldY = y - positions.oldY;
    if (oldY >= 0 && oldY < height) {
      const int left = positions.oldX > 0 ? positions.oldX : 0;
      const int sourceX = left - positions.oldX;
      const int span = width - (left + sourceX);
      if (span > 0)
        std::memcpy(target + left, oldFrame + size_t(oldY) * width + sourceX,
                    size_t(span) * sizeof(uint16_t));
    }
    const int newY = y - positions.newY;
    if (newY >= 0 && newY < height) {
      const int left = positions.newX > 0 ? positions.newX : 0;
      const int sourceX = left - positions.newX;
      const int span = width - (left + sourceX);
      if (span > 0)
        std::memcpy(target + left, newFrame + size_t(newY) * width + sourceX,
                    size_t(span) * sizeof(uint16_t));
    }
    target += stride;
  }
}
