#pragma once
// Native LVGL navigation direction and ring overlays. On the ESP32 these are
// enabled only after screen-geometry QA via MOTONAV_NATIVE_NAV_CUES, because a
// turn arrow must never silently change meaning during a UI migration.
#include <lvgl.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include "TurnCueGeometry.h"
#include "NavigationCueModel.h"

#ifndef MOTONAV_NATIVE_NAV_CUES
#define MOTONAV_NATIVE_NAV_CUES 0
#endif

struct MotoNavNavigationCues {
  lv_obj_t *root = nullptr;
  lv_obj_t *ring = nullptr;
  lv_obj_t *arrow = nullptr;
  lv_obj_t *tipLeft = nullptr;
  lv_obj_t *tipRight = nullptr;
  lv_obj_t *arrivalOuter = nullptr;
  lv_obj_t *arrivalInner = nullptr;
  lv_obj_t *distance = nullptr;
  lv_point_t uTurnPoints[5] = {};
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
  // Arrival is a distinct destination glyph, not a straight arrow.
  view.arrivalOuter = lv_obj_create(view.root);
  lv_obj_remove_style_all(view.arrivalOuter);
  lv_obj_set_pos(view.arrivalOuter, 150, 371);
  lv_obj_set_size(view.arrivalOuter, 54, 54);
  lv_obj_set_style_radius(view.arrivalOuter, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_color(view.arrivalOuter, lv_color_hex(0x21DE86), 0);
  lv_obj_set_style_border_width(view.arrivalOuter, 6, 0);
  lv_obj_set_style_bg_opa(view.arrivalOuter, LV_OPA_TRANSP, 0);
  lv_obj_clear_flag(view.arrivalOuter, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  view.arrivalInner = lv_obj_create(view.root);
  lv_obj_remove_style_all(view.arrivalInner);
  lv_obj_set_pos(view.arrivalInner, 169, 390);
  lv_obj_set_size(view.arrivalInner, 16, 16);
  lv_obj_set_style_radius(view.arrivalInner, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(view.arrivalInner, lv_color_hex(0x21DE86), 0);
  lv_obj_set_style_bg_opa(view.arrivalInner, LV_OPA_COVER, 0);
  lv_obj_clear_flag(view.arrivalInner, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(view.arrivalOuter, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(view.arrivalInner, LV_OBJ_FLAG_HIDDEN);
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
    MotoNavNavigationCues &view, MotoNavCueKind kind,
    float headingDeg, const char *distance,
    bool near, float progress, bool progressActive, bool showTrack,
    bool tripActive, lv_color_t color) {
  if (!view.root) return;
  const auto p = motonavTurnArrowGeometry(
      headingDeg, near ? 176.f : 185.f, near ? 403.f : 417.f);
  view.shaftPoints[0] = {static_cast<lv_coord_t>(std::lround(p.startX)),
                         static_cast<lv_coord_t>(std::lround(p.startY))};
  view.shaftPoints[1] = {static_cast<lv_coord_t>(std::lround(p.elbowX)),
                         static_cast<lv_coord_t>(std::lround(p.elbowY))};
  view.shaftPoints[2] = {static_cast<lv_coord_t>(std::lround(p.endX)),
                         static_cast<lv_coord_t>(std::lround(p.endY))};
  view.leftPoints[0] = view.shaftPoints[2];
  view.leftPoints[1] = {static_cast<lv_coord_t>(std::lround(p.leftX)),
                        static_cast<lv_coord_t>(std::lround(p.leftY))};
  view.rightPoints[0] = view.shaftPoints[2];
  view.rightPoints[1] = {static_cast<lv_coord_t>(std::lround(p.rightX)),
                         static_cast<lv_coord_t>(std::lround(p.rightY))};
  const bool arrived = kind == MotoNavCueKind::Arrive;
  const bool uTurn = kind == MotoNavCueKind::UTurn;
  if (arrived) {
    lv_obj_add_flag(view.arrow, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(view.tipLeft, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(view.tipRight, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(view.arrivalOuter, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(view.arrivalInner, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_clear_flag(view.arrow, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(view.tipLeft, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(view.tipRight, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(view.arrivalOuter, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(view.arrivalInner, LV_OBJ_FLAG_HIDDEN);
    if (uTurn) {
      // Deliberate counter-clockwise U-turn, independent of a dubious
      // heading angle from the phone.
      view.uTurnPoints[0] = {185, 431};
      view.uTurnPoints[1] = {185, 399};
      view.uTurnPoints[2] = {178, 387};
      view.uTurnPoints[3] = {162, 387};
      view.uTurnPoints[4] = {154, 401};
      view.leftPoints[0] = view.uTurnPoints[4];
      view.leftPoints[1] = {153, 382};
      view.rightPoints[0] = view.uTurnPoints[4];
      view.rightPoints[1] = {170, 401};
      lv_line_set_points(view.arrow, view.uTurnPoints, 5);
    } else {
      lv_line_set_points(view.arrow, view.shaftPoints, 3);
    }
    lv_line_set_points(view.tipLeft, view.leftPoints, 2);
    lv_line_set_points(view.tipRight, view.rightPoints, 2);
  }
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
