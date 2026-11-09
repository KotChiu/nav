#pragma once
// LVGL v8 image adapters for actual BLE bitmaps and RGB565 album art.
// This file is shared by ESP32-S3 and the SDL2 simulator.
#include <lvgl.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include "RasterBridge.h"

struct MotoNavRasterAsset {
  lv_obj_t *image = nullptr;
  lv_img_dsc_t descriptor = {};
  uint8_t *pixels = nullptr;
  size_t allocated = 0;
  void *(*allocate)(size_t) = nullptr;
  void (*release)(void *) = nullptr;
  uint32_t revision = UINT32_MAX;
  int width = 0, height = 0, top = 0;
  uint8_t tint = 0;
  bool ready = false;
};

static inline void motonavRasterSetHidden(lv_obj_t *obj, bool hidden) {
  if (!obj || lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN) == hidden) return;
  if (hidden) lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

static inline void motonavRasterCreate(MotoNavRasterAsset &asset,
                                       lv_obj_t *parent,
                                       void *(*allocate)(size_t) = std::malloc,
                                       void (*release)(void *) = std::free) {
  asset.allocate = allocate;
  asset.release = release;
  asset.image = lv_img_create(parent);
  lv_obj_clear_flag(asset.image, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  motonavRasterSetHidden(asset.image, true);
}

static inline void motonavRasterRelease(MotoNavRasterAsset &asset) {
  // Only after the LVGL object has been destroyed, or the display is stopped.
  if (asset.pixels && asset.release) asset.release(asset.pixels);
  asset.pixels = nullptr;
  asset.allocated = 0;
  asset.image = nullptr;
  asset.ready = false;
  asset.revision = UINT32_MAX;
  asset.descriptor = {};
}

static inline bool motonavRasterPrepare(MotoNavRasterAsset &asset, int width, int height) {
  if (!asset.image || !asset.allocate || !asset.release ||
      width <= 0 || height <= 0 || width > 466 || height > 466) return false;
  const size_t bytes = size_t(width) * height * LV_IMG_PX_SIZE_ALPHA_BYTE;
  if (bytes == 0 || bytes > 466U * 466U * LV_IMG_PX_SIZE_ALPHA_BYTE) return false;

  uint8_t *next = asset.pixels;
  if (bytes > asset.allocated) {
    next = static_cast<uint8_t *>(asset.allocate(bytes));
    if (!next) return false;
  }
  // The decoder may cache the descriptor pointer; evict before repointing it.
  lv_img_cache_invalidate_src(&asset.descriptor);
  if (next != asset.pixels) {
    uint8_t *old = asset.pixels;
    asset.pixels = next;
    asset.allocated = bytes;
    if (old) asset.release(old);
  }
  asset.descriptor.header.always_zero = 0;
  asset.descriptor.header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
  asset.descriptor.header.w = width;
  asset.descriptor.header.h = height;
  asset.descriptor.data_size = bytes;
  asset.descriptor.data = asset.pixels;
  asset.width = width;
  asset.height = height;
  return true;
}

static inline void motonavRasterPresent(MotoNavRasterAsset &asset, int x, int y) {
  if (!asset.image || !asset.pixels) return;
  if (lv_obj_get_x(asset.image) != x || lv_obj_get_y(asset.image) != y)
    lv_obj_set_pos(asset.image, x, y);
  lv_img_set_src(asset.image, &asset.descriptor);
  asset.ready = true;
}

// Source is the 4-bit grayscale image received over BLE, not an Arduino glyph.
// revision must increase whenever the BLE receiver replaces a source buffer.
static inline bool motonavRasterSetText(MotoNavRasterAsset &asset,
                                         const uint8_t *packed, size_t sourceCapacity,
                                         int sourceW, int sourceH, int top,
                                         uint8_t tint, uint32_t revision) {
  if (!packed || sourceW <= 0 || sourceH <= 0) {
    asset.ready = false;
    motonavRasterSetHidden(asset.image, true);
    return false;
  }
  const MotoNavBitmapBox rect = motonavBitmapBox(sourceW, sourceH, top);
  if (!rect.valid() || rect.y < 0 || rect.y + rect.height > 466) {
    asset.ready = false;
    motonavRasterSetHidden(asset.image, true);
    return false;
  }
  if (asset.ready && revision == asset.revision && rect.width == asset.width &&
      rect.height == asset.height && top == asset.top && tint == asset.tint) return true;
  if (!motonavRasterPrepare(asset, rect.width, rect.height)) {
    asset.ready = false;
    motonavRasterSetHidden(asset.image, true);
    return false;
  }
  const bool rendered = motonavRasterTextGray(
      packed, sourceCapacity, sourceW, sourceH, rect.width, rect.height, tint,
      [&](int x, int y, uint8_t gray) {
        lv_img_buf_set_px_color(&asset.descriptor, x, y, lv_color_make(gray, gray, gray));
        lv_img_buf_set_px_alpha(&asset.descriptor, x, y,
                                gray ? LV_OPA_COVER : LV_OPA_TRANSP);
      });
  if (!rendered) {
    asset.ready = false;
    motonavRasterSetHidden(asset.image, true);
    return false;
  }
  asset.revision = revision;
  asset.top = top;
  asset.tint = tint;
  motonavRasterPresent(asset, rect.x, rect.y);
  return true;
}

struct MotoNavCoverAsset {
  MotoNavRasterAsset raster;
  lv_obj_t *halo = nullptr;
};

static inline void motonavCoverCreate(MotoNavCoverAsset &cover, lv_obj_t *parent,
                                      void *(*allocate)(size_t) = std::malloc,
                                      void (*release)(void *) = std::free) {
  cover.halo = lv_obj_create(parent);
  lv_obj_remove_style_all(cover.halo);
  lv_obj_set_pos(cover.halo, 233 - 67, 110 - 67);
  lv_obj_set_size(cover.halo, 134, 134);
  lv_obj_set_style_radius(cover.halo, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(cover.halo, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(cover.halo, lv_color_hex(0x15232C), 0);
  lv_obj_clear_flag(cover.halo, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  motonavRasterSetHidden(cover.halo, true);
  motonavRasterCreate(cover.raster, parent, allocate, release);
}

static inline bool motonavCoverSet(MotoNavCoverAsset &cover, const uint16_t *pixels,
                                    int srcW, int srcH, uint32_t revision) {
  MotoNavRasterAsset &asset = cover.raster;
  if (!pixels || srcW <= 0 || srcH <= 0) {
    asset.ready = false;
    motonavRasterSetHidden(asset.image, true);
    motonavRasterSetHidden(cover.halo, true);
    return false;
  }
  if (asset.ready && asset.revision == revision &&
      asset.width == 128 && asset.height == 128) return true;
  if (!motonavRasterPrepare(asset, 128, 128)) {
    asset.ready = false;
    motonavRasterSetHidden(asset.image, true);
    motonavRasterSetHidden(cover.halo, true);
    return false;
  }
  if (!motonavRasterCover(pixels, srcW, srcH, 128, [&](int x, int y,
                                                         uint16_t color, bool inside) {
    const uint8_t red = uint8_t(((color >> 11) & 31) * 255 / 31);
    const uint8_t green = uint8_t(((color >> 5) & 63) * 255 / 63);
    const uint8_t blue = uint8_t((color & 31) * 255 / 31);
    lv_img_buf_set_px_color(&asset.descriptor, x, y, lv_color_make(red, green, blue));
    lv_img_buf_set_px_alpha(&asset.descriptor, x, y,
                            inside ? LV_OPA_COVER : LV_OPA_TRANSP);
  })) {
    asset.ready = false;
    return false;
  }
  asset.revision = revision;
  motonavRasterPresent(asset, 233 - 64, 110 - 64);
  return true;
}

static inline void motonavCoverVisible(MotoNavCoverAsset &cover, bool visible) {
  motonavRasterSetHidden(cover.halo, !visible || !cover.raster.ready);
  motonavRasterSetHidden(cover.raster.image, !visible || !cover.raster.ready);
}
