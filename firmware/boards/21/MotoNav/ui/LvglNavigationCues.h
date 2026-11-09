#pragma once
// Native LVGL navigation direction and ring overlays. On the ESP32 these are
// enabled only after screen-geometry QA via MOTONAV_NATIVE_NAV_CUES, because a
// turn arrow must never silently change meaning during a UI migration.
#include <lvgl.h>
#include <cmath>
#include <cstdio>
#include <cstring>

#ifndef MOTONAV_NATIVE_NAV_CUES
#define MOTONAV_NATIVE_NAV_CUES 0
#endif

struct MotoNavNavigationCues {
  lv_obj_t *root = nullptr;
  lv_obj_t *ring = nullptr;
  lv_obj_t *arrow = nullptr;
  lv_obj_t *tipLeft = nullptr;
  lv_obj_t *tipRight = nullptr;
  lv_obj_t *distance = nullptr;
  lv_point_t shaftPoints[3] = {};
  lv_point_t leftPoints[2] = {};
  lv_point_t rightPoints[2] = {};
};

static inline void motonavNavigationCuesCreate(MotoNavNavigationCues &view,
                                                lv_obj_t *parent) {
  if (!parent) return;
  view.root = lv_obj_create(parent);
  lv_obj_remove_style_all(view.root);
  lv_obj_set_pos(view.root, 0, 0);
  lv_obj_set_size(view.root, 466, 466);
  lv_obj_clear_flag(view.root, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

  view.ring = lv_arc_create(view.root);
  lv_obj_set_pos(view.ring, 13, 13);
  lv_obj_set_size(view.ring, 440, 440);
  lv_obj_clear_flag(view.ring, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  lv_arc_set_range(view.ring, 0, 1000);
  lv_arc_set_rotation(view.ring, 270);
  lv_arc_set_bg_angles(view.ring, 0, 360);
  lv_obj_set_style_bg_opa(view.ring, LV_OPA_TRANSP, LV_PART_KNOB);
  lv_obj_set_style_arc_width(view.ring, 10, LV_PART_MAIN);
  lv_obj_set_style_arc_color(view.ring, lv_color_hex(0x29323A), LV_PART_MAIN);
  lv_obj_set_style_arc_width(view.ring, 10, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(view.ring, lv_color_hex(0xFDE15A), LV_PART_INDICATOR);
  lv_arc_set_value(view.ring, 0);

  view.arrow = lv_line_create(view.root);
  view.tipLeft = lv_line_create(view.root);
  view.tipRight = lv_line_create(view.root);
  lv_obj_t *segments[] = {view.arrow, view.tipLeft, view.tipRight};
  for (lv_obj_t *segment : segments) {
    lv_obj_set_size(segment, 466, 466);
    lv_obj_set_pos(segment, 0, 0);
    lv_obj_set_style_line_width(segment, 8, 0);
    lv_obj_set_style_line_rounded(segment, true, 0);
    lv_obj_set_style_line_color(segment, lv_color_white(), 0);
    lv_obj_clear_flag(segment, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  }
  view.distance = lv_label_create(view.root);
  lv_label_set_long_mode(view.distance, LV_LABEL_LONG_CLIP);
  lv_obj_set_size(view.distance, 140, 46);
  lv_obj_set_pos(view.distance, 233, 368);
  lv_obj_set_style_text_align(view.distance, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(view.distance, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(view.distance, lv_color_white(), 0);
  lv_obj_add_flag(view.root, LV_OBJ_FLAG_HIDDEN);
}

static inline void motonavNavigationCuesUpdate(
    MotoNavNavigationCues &view, float headingDeg, const char *distance,
    bool near, float progress, bool progressActive, bool showTrack,
    bool tripActive, lv_color_t color) {
  if (!view.root) return;
  // Screen coordinates: 0 degrees points north, positive turns to the right.
  const float a = headingDeg * 3.14159265359f / 180;
  const float sx = std::sin(a), cy = -std::cos(a);
  const float x = near ? 176.f : 185.f, y = near ? 403.f : 417.f;
  const float ex = x + 40 * sx, ey = y + 40 * cy;
  const float lx = ex - 14 * sx - 10 * cy, ly = ey - 14 * cy + 10 * sx;
  const float rx = ex - 14 * sx + 10 * cy, ry = ey - 14 * cy - 10 * sx;
  view.shaftPoints[0] = {static_cast<lv_coord_t>(x),
                         static_cast<lv_coord_t>(y + 5)};
  view.shaftPoints[1] = {static_cast<lv_coord_t>(x),
                         static_cast<lv_coord_t>(y - 8)};
  view.shaftPoints[2] = {static_cast<lv_coord_t>(std::lround(ex)),
                         static_cast<lv_coord_t>(std::lround(ey))};
  view.leftPoints[0] = view.shaftPoints[2];
  view.leftPoints[1] = {static_cast<lv_coord_t>(std::lround(lx)),
                        static_cast<lv_coord_t>(std::lround(ly))};
  view.rightPoints[0] = view.shaftPoints[2];
  view.rightPoints[1] = {static_cast<lv_coord_t>(std::lround(rx)),
                         static_cast<lv_coord_t>(std::lround(ry))};
  lv_line_set_points(view.arrow, view.shaftPoints, 3);
  lv_line_set_points(view.tipLeft, view.leftPoints, 2);
  lv_line_set_points(view.tipRight, view.rightPoints, 2);
  lv_color_t textColor = color;
  for (lv_obj_t *segment : {view.arrow, view.tipLeft, view.tipRight}) {
    lv_obj_set_style_line_color(segment, textColor, 0);
  }
  const char *txt = distance ? distance : "";
  if (std::strcmp(lv_label_get_text(view.distance), txt) != 0)
    lv_label_set_text(view.distance, txt);
  lv_obj_set_style_text_color(view.distance, textColor, 0);
  if (lv_obj_get_y(view.distance) != (near ? 360 : 374))
    lv_obj_set_y(view.distance, near ? 360 : 374);

  const bool ringVisible = progressActive || tripActive || showTrack;
  if (ringVisible) {
    int value = static_cast<int>(std::lround(std::fmax(0.0f,
                std::fmin(1.0f, progress)) * 1000));
    if (lv_arc_get_value(view.ring) != value) lv_arc_set_value(view.ring, value);
    lv_obj_set_style_arc_width(view.ring, near ? 14 : 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(view.ring, color, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(view.ring,
      tripActive ? lv_color_hex(0x26E8) : lv_color_hex(0x29323A),
      LV_PART_MAIN);
    lv_obj_clear_flag(view.ring, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(view.ring, LV_OBJ_FLAG_HIDDEN);
  }
}

static inline void motonavNavigationCuesVisible(
    MotoNavNavigationCues &view, bool visible) {
  if (!view.root) return;
  bool hidden = lv_obj_has_flag(view.root, LV_OBJ_FLAG_HIDDEN);
  if (visible && hidden) {
    lv_obj_move_foreground(view.root);
    lv_obj_clear_flag(view.root, LV_OBJ_FLAG_HIDDEN);
  } else if (!visible && !hidden) lv_obj_add_flag(view.root, LV_OBJ_FLAG_HIDDEN);
}
