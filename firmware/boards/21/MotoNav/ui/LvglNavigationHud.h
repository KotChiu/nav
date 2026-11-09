#pragma once
// Native LVGL 8.3 navigation speed HUD, shared by ESP32 and desktop simulator.
// The underlying map polygons, road traffic and map tiles are intentionally
// not owned by this view: they are the next migration stage.
#include <lvgl.h>
#include <cstdio>
#include <cstring>

struct MotoNavNavigationHud {
  lv_obj_t *root = nullptr;
  lv_obj_t *speed = nullptr;
  lv_obj_t *units = nullptr;
  lv_obj_t *limitRing = nullptr;
  lv_obj_t *limit = nullptr;
  lv_obj_t *distance = nullptr;
};

static inline void motonavNavHudText(lv_obj_t *label, const char *value) {
  if (!label) return;
  const char *current = lv_label_get_text(label);
  if (!current || strcmp(current, value ? value : "") != 0)
    lv_label_set_text(label, value ? value : "");
}
static inline void motonavNavHudHidden(lv_obj_t *object, bool hidden) {
  if (!object || lv_obj_has_flag(object, LV_OBJ_FLAG_HIDDEN) == hidden) return;
  if (hidden) lv_obj_add_flag(object, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_clear_flag(object, LV_OBJ_FLAG_HIDDEN);
}
static inline lv_obj_t *motonavNavHudLabel(
    lv_obj_t *parent, int w, int h, const lv_font_t *font,
    lv_color_t color) {
  lv_obj_t *label = lv_label_create(parent);
  lv_obj_set_size(label, w, h);
  lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(label, font, 0);
  lv_obj_set_style_text_color(label, color, 0);
  return label;
}
static inline void motonavNavigationHudCreate(
    MotoNavNavigationHud &view, lv_obj_t *parent) {
  view.root = lv_obj_create(parent);
  lv_obj_remove_style_all(view.root);
  lv_obj_set_pos(view.root, 0, 0);
  lv_obj_set_size(view.root, 466, 120);
  lv_obj_set_style_bg_opa(view.root, LV_OPA_TRANSP, 0);
  lv_obj_clear_flag(view.root, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  view.speed = motonavNavHudLabel(view.root, 180, 54,
                                   &lv_font_montserrat_40, lv_color_white());
  view.units = motonavNavHudLabel(view.root, 90, 22,
                                   &lv_font_montserrat_14, lv_color_white());
  view.limitRing = lv_obj_create(view.root);
  lv_obj_remove_style_all(view.limitRing);
  lv_obj_set_size(view.limitRing, 50, 50);
  lv_obj_set_pos(view.limitRing, 260, 35);
  lv_obj_set_style_radius(view.limitRing, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(view.limitRing, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(view.limitRing, lv_color_white(), 0);
  lv_obj_set_style_border_width(view.limitRing, 7, 0);
  lv_obj_set_style_border_color(view.limitRing, lv_color_hex(0xE63340), 0);
  lv_obj_clear_flag(view.limitRing, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  view.limit = motonavNavHudLabel(view.limitRing, 50, 28,
                                   &lv_font_montserrat_20, lv_color_black());
  lv_obj_set_pos(view.limit, 0, 13);
  view.distance = motonavNavHudLabel(view.root, 112, 20,
                                      &lv_font_montserrat_14, lv_color_white());
  motonavNavHudHidden(view.root, true);
}

static inline void motonavNavigationHudUpdate(
    MotoNavNavigationHud &view, int speedKmh, bool cameraVisible,
    int speedLimit, const char *cameraDistance, bool overspeed,
    lv_color_t normalColor, lv_color_t alertColor) {
  if (!view.root) return;
  char buffer[24];
  const bool validSpeed = speedKmh >= 0;
  if (validSpeed) {
    snprintf(buffer, sizeof(buffer), "%d", speedKmh);
    motonavNavHudText(view.speed, buffer);
  }
  motonavNavHudHidden(view.speed, !validSpeed);
  motonavNavHudHidden(view.units, !validSpeed);
  lv_color_t current = overspeed ? alertColor : normalColor;
  if (lv_color_to16(lv_obj_get_style_text_color(view.speed, 0)) != lv_color_to16(current))
    lv_obj_set_style_text_color(view.speed, current, 0);
  if (lv_color_to16(lv_obj_get_style_text_color(view.units, 0)) != lv_color_to16(current))
    lv_obj_set_style_text_color(view.units, current, 0);
  const int sx = cameraVisible ? 233 - 40 : 233;
  const int sx0 = sx - 90;
  if (lv_obj_get_x(view.speed) != sx0) lv_obj_set_x(view.speed, sx0);
  if (lv_obj_get_y(view.speed) != 35) lv_obj_set_y(view.speed, 35);
  if (lv_obj_get_x(view.units) != sx - 45) lv_obj_set_x(view.units, sx - 45);
  if (lv_obj_get_y(view.units) != 82) lv_obj_set_y(view.units, 82);
  motonavNavHudText(view.units, "km/h");

  const bool limitValid = cameraVisible && speedLimit > 0;
  motonavNavHudHidden(view.limitRing, !limitValid);
  motonavNavHudHidden(view.distance, !cameraVisible);
  if (limitValid) {
    snprintf(buffer, sizeof(buffer), "%d", speedLimit);
    motonavNavHudText(view.limit, buffer);
  }
  if (cameraVisible) motonavNavHudText(view.distance, cameraDistance);
  if (lv_obj_get_x(view.distance) != 252) lv_obj_set_x(view.distance, 252);
  if (lv_obj_get_y(view.distance) != 86) lv_obj_set_y(view.distance, 86);
}

static inline void motonavNavigationHudVisible(
    MotoNavNavigationHud &view, bool visible) {
  if (!view.root) return;
  if (visible) lv_obj_move_foreground(view.root);
  motonavNavHudHidden(view.root, !visible);
}
