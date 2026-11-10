#pragma once
// MotoNav visual language • Carbon Racing / 480x480 circular display.
// All text uses the same LVGL Montserrat family (16/20/24/28/40/48).
// Graphite, warm white, racing orange and restrained ice blue.\n// Do not import bitmap "watch digits" or old pixel rider resources.
#include <lvgl.h>

namespace MotoNavUI {
static constexpr uint32_t Background = 0x000000;
static constexpr uint32_t Surface = 0x191B21;
static constexpr uint32_t SurfaceRaised = 0x242730;
static constexpr uint32_t Line = 0x343740;
static constexpr uint32_t Text = 0xF5F3EE;
static constexpr uint32_t Muted = 0xB8BBC5;
static constexpr uint32_t Faint = 0x858996;
static constexpr uint32_t Accent = 0xFF895C;
static constexpr uint32_t Cool = 0x8CCBFF;
static constexpr uint32_t Warm = 0xF4BA72;
static constexpr uint32_t Red = 0xFF657A;
static constexpr int Width = 466;
static constexpr int Center = 233;

inline lv_color_t color(uint32_t rgb) { return lv_color_hex(rgb); }
inline void noInteraction(lv_obj_t *obj) {
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
}
inline lv_obj_t *label(lv_obj_t *parent, const char *value,
                        const lv_font_t *font, uint32_t rgb) {
  lv_obj_t *l = lv_label_create(parent);
  lv_label_set_text(l, value);
  lv_obj_set_style_text_font(l, font, 0);
  lv_obj_set_style_text_color(l, color(rgb), 0);
  lv_obj_set_style_bg_opa(l, LV_OPA_TRANSP, 0);
  lv_obj_set_style_pad_all(l, 0, 0);
  noInteraction(l);
  return l;
}
inline lv_obj_t *panel(lv_obj_t *parent, int x, int y, int w, int h,
                        int radius = 22, uint32_t background = Surface) {
  lv_obj_t *p = lv_obj_create(parent);
  lv_obj_set_pos(p, x, y);
  lv_obj_set_size(p, w, h);
  lv_obj_set_style_radius(p, radius, 0);
  lv_obj_set_style_border_width(p, 1, 0);
  lv_obj_set_style_border_color(p, color(Line), 0);
  lv_obj_set_style_bg_color(p, color(background), 0);
  lv_obj_set_style_bg_opa(p, LV_OPA_COVER, 0);
  lv_obj_set_style_shadow_width(p, 0, 0);
  lv_obj_set_style_pad_all(p, 0, 0);
  noInteraction(p);
  return p;
}
inline void capsule(lv_obj_t *obj, uint32_t background, uint32_t foreground) {
  lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(obj, color(background), 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(obj, 0, 0);
  lv_obj_set_style_text_color(obj, color(foreground), 0);
  lv_obj_set_style_pad_hor(obj, 14, 0);
  lv_obj_set_style_pad_ver(obj, 7, 0);
}
inline lv_obj_t *button(lv_obj_t *parent, int x, int y, int w, int h,
                        const char *value, uint32_t background = SurfaceRaised) {
  lv_obj_t *b = lv_btn_create(parent);
  lv_obj_set_pos(b, x, y);
  lv_obj_set_size(b, w, h);
  lv_obj_set_style_radius(b, 14, 0);
  lv_obj_set_style_border_width(b, 1, 0);
  lv_obj_set_style_border_color(b, color(Line), 0);
  lv_obj_set_style_bg_color(b, color(background), 0);
  lv_obj_set_style_bg_color(b, color(Accent), LV_STATE_PRESSED);
  lv_obj_set_style_shadow_width(b, 0, 0);
  lv_obj_set_style_pad_all(b, 0, 0);
  lv_obj_clear_flag(b, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *l = label(b, value, &lv_font_montserrat_16, Text);
  lv_obj_center(l);
  return b;
}
inline lv_obj_t *arc(lv_obj_t *parent, int x, int y, int size,
                      int start, int end, uint32_t tint, int thickness = 3) {
  lv_obj_t *a = lv_arc_create(parent);
  lv_obj_set_pos(a, x, y);
  lv_obj_set_size(a, size, size);
  lv_arc_set_bg_angles(a, start, end);
  lv_arc_set_range(a, 0, 100);
  lv_arc_set_value(a, 0);
  lv_obj_set_style_arc_width(a, thickness, LV_PART_MAIN);
  lv_obj_set_style_arc_color(a, color(tint), LV_PART_MAIN);
  lv_obj_set_style_arc_opa(a, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_arc_opa(a, LV_OPA_TRANSP, LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(a, LV_OPA_TRANSP, LV_PART_KNOB);
  noInteraction(a);
  return a;
}
}
