#pragma once
// Framebuffer ownership contract shared by device code and host tests.
// The CPU may mutate the logical buffer or the LVGL Map Canvas only when its
// bytes are not being read as a transport snapshot by the RGB flush task.
// An LVGL image source must not alias the destination of LVGL's draw pipeline.
#include <cstddef>
#include <cstdint>

enum class MotoNavFrameRole : uint8_t {
  LogicalDraw = 0, // UI draw destination, always mutable on main task
  MapSource = 1,   // complete LVGL image/canvas, not the draw destination
  Transport = 2,   // immutable while DMA/VSYNC frame submission is in flight
};

static inline bool motonavFramePointersIndependent(
    const void *logical, const void *mapSource,
    const void *transport, const void *inFlight) {
  return logical && mapSource && mapSource != logical &&
         mapSource != transport && mapSource != inFlight;
}

static inline size_t motonavFrameBytes(int width, int height) {
  if (width <= 0 || height <= 0 || width > 1024 || height > 1024) return 0;
  return static_cast<size_t>(width) * static_cast<size_t>(height) *
         sizeof(uint16_t);
}
