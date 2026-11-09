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

// Page snapshots can carry a one-pixel edge tint on the panel. Pull the two
// seam-adjacent pixels one step inward so that edge tint cannot become a line.
static inline void motonavSealPageSlideSeam(
    uint16_t *target, size_t stride, int width, int height,
    const MotoNavSlidePositions &positions) {
  if (positions.oldY == positions.newY) {
    const int boundary = positions.oldX > positions.newX
      ? positions.oldX : positions.newX;
    if (boundary <= 1 || boundary >= width - 1) return;
    for (int y = 0; y < height; ++y) {
      uint16_t *row = target + size_t(y) * stride;
      row[boundary - 1] = row[boundary - 2];
      row[boundary] = row[boundary + 1];
    }
    return;
  }
  if (positions.oldX == positions.newX) {
    const int boundary = positions.oldY > positions.newY
      ? positions.oldY : positions.newY;
    if (boundary <= 1 || boundary >= height - 1) return;
    memcpy(target + size_t(boundary - 1) * stride,
           target + size_t(boundary - 2) * stride,
           size_t(width) * sizeof(uint16_t));
    memcpy(target + size_t(boundary) * stride,
           target + size_t(boundary + 1) * stride,
           size_t(width) * sizeof(uint16_t));
  }
}
