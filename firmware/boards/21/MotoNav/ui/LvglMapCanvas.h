#pragma once
// Dedicated LVGL 8.3 true-color map canvas. The backing RGB565 pixels are
// never the display draw buffer nor the DMA flush snapshot. The legacy map
// renderer can temporarily target this storage while vector portions are
// replaced by native LVGL canvas draw operations.
#include <lvgl.h>
#include <cmath>
#include <cstdint>

#ifndef MOTONAV_NATIVE_MAP_CANVAS
#define MOTONAV_NATIVE_MAP_CANVAS 1
#endif

struct MotoNavMapCanvas {
  lv_obj_t *canvas = nullptr;
  uint16_t *pixels = nullptr;
  int width = 0;
  int height = 0;
  bool drawing = false;
  uint32_t commits = 0;
  uint32_t nativeLines = 0;
  uint32_t nativePolygons = 0;
};

static inline bool motonavMapCanvasCreate(MotoNavMapCanvas &map,
                                           lv_obj_t *parent,
                                           uint16_t *pixels,
                                           int width, int height) {
  if (!parent || !pixels || width <= 0 || height <= 0 ||
      width > 1024 || height > 1024) return false;
  lv_obj_t *canvas = lv_canvas_create(parent);
  if (!canvas) return false;
  lv_canvas_set_buffer(canvas, pixels, width, height, LV_IMG_CF_TRUE_COLOR);
  lv_obj_set_pos(canvas, 0, 0);
  lv_obj_clear_flag(canvas, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_move_background(canvas); // below the real navigation HUD and widgets
  lv_obj_add_flag(canvas, LV_OBJ_FLAG_HIDDEN);
  map.canvas = canvas;
  map.pixels = pixels;
  map.width = width;
  map.height = height;
  return true;
}

static inline bool motonavMapCanvasBegin(MotoNavMapCanvas &map) {
  if (!map.canvas || !map.pixels || map.drawing) return false;
  map.drawing = true;
  return true;
}
static inline void motonavMapCanvasCommit(MotoNavMapCanvas &map) {
  if (!map.drawing) return;
  map.drawing = false;
  ++map.commits;
  lv_obj_invalidate(map.canvas);
}
static inline void motonavMapCanvasVisible(MotoNavMapCanvas &map, bool visible) {
  if (!map.canvas) return;
  bool hidden = lv_obj_has_flag(map.canvas, LV_OBJ_FLAG_HIDDEN);
  if (visible && hidden) lv_obj_clear_flag(map.canvas, LV_OBJ_FLAG_HIDDEN);
  else if (!visible && !hidden) lv_obj_add_flag(map.canvas, LV_OBJ_FLAG_HIDDEN);
}

static inline bool motonavMapCanvasPoints(
    const float *x, const float *y, int count, lv_point_t *points,
    int capacity, int width, int height) {
  if (!x || !y || !points || count < 2 || count > capacity) return false;
  for (int i = 0; i < count; ++i) {
    // Do not convert unbounded projected coordinates to LVGL int16_t.
    // The legacy renderer handles huge or non-finite off-screen segments.
    if (!std::isfinite(x[i]) || !std::isfinite(y[i]) ||
        x[i] < -width || x[i] > 2 * width ||
        y[i] < -height || y[i] > 2 * height) return false;
    points[i].x = static_cast<lv_coord_t>(std::lround(x[i]));
    points[i].y = static_cast<lv_coord_t>(std::lround(y[i]));
  }
  return true;
}

static inline bool motonavMapCanvasPolyline(
    MotoNavMapCanvas &map, const float *x, const float *y,
    int count, float width, uint16_t color, bool caps) {
  if (!map.drawing || !map.canvas || count < 2 || count > 96 ||
      !std::isfinite(width) || width < 1 || width > 80) return false;
  lv_point_t points[96];
  if (!motonavMapCanvasPoints(x, y, count, points, 96,
                              map.width, map.height)) return false;
  lv_draw_line_dsc_t dsc;
  lv_draw_line_dsc_init(&dsc);
  dsc.color.full = color;
  dsc.width = static_cast<lv_coord_t>(std::lround(width));
  dsc.opa = LV_OPA_COVER;
  dsc.round_start = caps;
  dsc.round_end = caps;
  lv_canvas_draw_line(map.canvas, points, count, &dsc);
  ++map.nativeLines;
  return true;
}

static inline bool motonavMapCanvasPolygon(
    MotoNavMapCanvas &map, const float *x, const float *y,
    int count, uint16_t color) {
  if (!map.drawing || !map.canvas || count < 3 || count > 96) return false;
  lv_point_t points[96];
  if (!motonavMapCanvasPoints(x, y, count, points, 96,
                              map.width, map.height)) return false;
  lv_draw_rect_dsc_t dsc;
  lv_draw_rect_dsc_init(&dsc);
  dsc.bg_color.full = color;
  dsc.bg_opa = LV_OPA_COVER;
  dsc.border_width = 0;
  lv_canvas_draw_polygon(map.canvas, points, count, &dsc);
  ++map.nativePolygons;
  return true;
}
