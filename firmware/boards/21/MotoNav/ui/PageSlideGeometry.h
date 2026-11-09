#pragma once
// Position-only transition model. Intentionally no LVGL/Arduino dependency.
// A complete opaque old+new frame pair covers the entire viewport at any p.
struct MotoNavSlidePositions {
  int oldX = 0, oldY = 0;
  int newX = 0, newY = 0;
};

static inline MotoNavSlidePositions motonavSlidePositions(
    int amount, bool horizontal, int direction, int width, int height) {
  MotoNavSlidePositions result;
  if (width <= 0 || height <= 0 || direction == 0) return result;
  const int extent = horizontal ? width : height;
  if (amount < 0) amount = 0;
  if (amount > extent) amount = extent;
  const int sign = direction > 0 ? 1 : -1;
  const int oldOffset = sign * amount;
  const int newOffset = sign * (amount - extent);
  if (horizontal) {
    result.oldX = oldOffset;
    result.newX = newOffset;
  } else {
    result.oldY = oldOffset;
    result.newY = newOffset;
  }
  return result;
}
