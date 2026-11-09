#pragma once
// Native LVGL 8.3 home ring: shared by the 2.1 firmware and desktop simulator.
// No Arduino, ESP32, DMA, PSRAM or board pin dependencies are allowed here.
#include <lvgl.h>

struct MotoNavHomeChrome {
  lv_obj_t *segments[4] = {nullptr, nullptr, nullptr, nullptr};
  lv_obj_t *topDot = nullptr;
};

static inline void motonavHomeChromeCreate(
    MotoNavHomeChrome &chrome, lv_obj_t *parent,
    int logicalCenter, uint16_t accent565, uint16_t accent2565) {
  // The old Arduino Canvas drew four independent 54-degree ring segments.
  // Native LVGL arcs replace those background pixels with invalidatable widgets.
  for (int i = 0; i < 4; ++i) {
    lv_obj_t *arc = lv_arc_create(parent);
    lv_obj_remove_style_all(arc);
    lv_obj_set_size(arc, 422, 422);
    lv_obj_set_pos(arc, logicalCenter - 211, logicalCenter - 211);
    lv_arc_set_mode(arc, LV_ARC_MODE_NORMAL);
    lv_arc_set_bg_angles(arc, 0, 360);
    lv_arc_set_angles(arc, 18 + i * 90, 72 + i * 90);
    lv_obj_set_style_arc_opa(arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 3, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, lv_color_make(
        (accent565 >> 11) * 255 / 31,
        ((accent565 >> 5) & 63) * 255 / 63,
        (accent565 & 31) * 255 / 31), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(arc, LV_OBJ_FLAG_HIDDEN);
    chrome.segments[i] = arc;
  }
  lv_obj_t *dot = lv_obj_create(parent);
  lv_obj_remove_style_all(dot);
  lv_obj_set_size(dot, 10, 10);
  lv_obj_set_pos(dot, logicalCenter - 5, 22);
  lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(dot, lv_color_make(
      (accent2565 >> 11) * 255 / 31,
      ((accent2565 >> 5) & 63) * 255 / 63,
      (accent2565 & 31) * 255 / 31), 0);
  lv_obj_clear_flag(dot, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(dot, LV_OBJ_FLAG_HIDDEN);
  chrome.topDot = dot;
}

static inline void motonavHomeChromeVisible(MotoNavHomeChrome &chrome, bool visible) {
  for (lv_obj_t *arc : chrome.segments) {
    if (!arc) continue;
    if (visible) lv_obj_clear_flag(arc, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(arc, LV_OBJ_FLAG_HIDDEN);
  }
  if (chrome.topDot) {
    if (visible) lv_obj_clear_flag(chrome.topDot, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(chrome.topDot, LV_OBJ_FLAG_HIDDEN);
  }
}
