#pragma once
// Shared vector icons: line artwork (no emoji, icon font or binary assets).
// All coordinate arrays have static lifetime as required by LVGL 8 lv_line.
#include <lvgl.h>
#include <cstdint>

namespace MotoNavUI {
enum class Icon : uint8_t {
  Map, Route, Weather, Music, Stocks, Settings, Screen,
  Ride, Link, Photo, ChevronLeft, ChevronRight, ChevronDown,
  Battery, TrendUp, TrendDown
};

inline lv_obj_t *iconStroke(lv_obj_t *root, const lv_point_t *pts,
                             uint16_t count, uint32_t ink, int weight) {
  lv_obj_t *line = lv_line_create(root);
  lv_line_set_points(line, pts, count);
  lv_obj_set_pos(line, 0, 0);
  lv_obj_set_style_line_color(line, lv_color_hex(ink), 0);
  lv_obj_set_style_line_width(line, weight, 0);
  lv_obj_set_style_line_rounded(line, true, 0);
  lv_obj_clear_flag(line, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  return line;
}

inline void iconRing(lv_obj_t *root, int x, int y, int diameter, uint32_t ink,
                     int weight = 2) {
  lv_obj_t *ring = lv_obj_create(root);
  lv_obj_remove_style_all(ring);
  lv_obj_set_pos(ring, x, y);
  lv_obj_set_size(ring, diameter, diameter);
  lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(ring, weight, 0);
  lv_obj_set_style_border_color(ring, lv_color_hex(ink), 0);
  lv_obj_set_style_bg_opa(ring, LV_OPA_TRANSP, 0);
  lv_obj_clear_flag(ring, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
}

// Coordinates are designed within a 24x24 logical glyph square. The returned
// root is reusable in buttons, chips, weather cards and map-mode HUD labels.
inline lv_obj_t *icon(lv_obj_t *parent, Icon kind, int x, int y,
                       uint32_t ink, int weight = 2) {
  lv_obj_t *root = lv_obj_create(parent);
  lv_obj_remove_style_all(root);
  lv_obj_set_pos(root, x, y);
  lv_obj_set_size(root, 24, 24);
  lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
  static const lv_point_t mapOutline[] = {{3,5},{9,3},{15,5},{21,3},{21,19},{15,21},{9,19},{3,21},{3,5}};
  static const lv_point_t mapFold1[] = {{9,3},{9,19}};
  static const lv_point_t mapFold2[] = {{15,5},{15,21}};
  static const lv_point_t routeLine[] = {{5,19},{5,13},{8,11},{16,11},{19,8}};
  static const lv_point_t routeArrow[] = {{14,4},{19,8},{15,12}};
  static const lv_point_t sun1[] = {{12,2},{12,5}};
  static const lv_point_t sun2[] = {{12,19},{12,22}};
  static const lv_point_t sun3[] = {{2,12},{5,12}};
  static const lv_point_t sun4[] = {{19,12},{22,12}};
  static const lv_point_t sun5[] = {{5,5},{7,7}};
  static const lv_point_t sun6[] = {{17,17},{19,19}};
  static const lv_point_t note[] = {{10,16},{10,5},{19,3},{19,14}};
  static const lv_point_t trendUp[] = {{3,18},{9,12},{13,15},{21,6}};
  static const lv_point_t trendDown[] = {{3,6},{9,12},{13,9},{21,18}};
  static const lv_point_t trendTipUp[] = {{15,6},{21,6},{21,12}};
  static const lv_point_t trendTipDown[] = {{15,18},{21,18},{21,12}};
  static const lv_point_t sliderA[] = {{3,6},{21,6}};
  static const lv_point_t sliderB[] = {{3,12},{21,12}};
  static const lv_point_t sliderC[] = {{3,18},{21,18}};
  static const lv_point_t screen[] = {{3,5},{21,5},{21,17},{3,17},{3,5}};
  static const lv_point_t screenStand[] = {{12,17},{12,21},{7,21},{17,21}};
  static const lv_point_t dash[] = {{4,17},{6,11},{12,8},{18,11},{20,17}};
  static const lv_point_t needle[] = {{12,17},{17,11}};
  static const lv_point_t link1[] = {{8,4},{8,20},{16,14},{5,8}};
  static const lv_point_t link2[] = {{8,4},{16,10},{5,16},{8,20}};
  static const lv_point_t photoBox[] = {{3,5},{21,5},{21,19},{3,19},{3,5}};
  static const lv_point_t photoHill[] = {{5,17},{10,12},{13,15},{16,12},{20,17}};
  static const lv_point_t left[] = {{16,4},{8,12},{16,20}};
  static const lv_point_t right[] = {{8,4},{16,12},{8,20}};
  static const lv_point_t down[] = {{4,8},{12,16},{20,8}};
  static const lv_point_t battery[] = {{3,6},{18,6},{18,18},{3,18},{3,6}};
  static const lv_point_t batteryTip[] = {{20,9},{21,9},{21,15},{20,15}};
  static const lv_point_t batteryFill[] = {{6,9},{14,9},{14,15},{6,15},{6,9}};
  auto stroke = [&](const lv_point_t *p, uint16_t n) {
    iconStroke(root, p, n, ink, weight);
  };
  switch (kind) {
    case Icon::Map:
      stroke(mapOutline, 9); stroke(mapFold1, 2); stroke(mapFold2, 2); break;
    case Icon::Route:
      iconRing(root, 2, 17, 4, ink, 2);
      stroke(routeLine, 5); stroke(routeArrow, 3); break;
    case Icon::Weather:
      iconRing(root, 7, 7, 10, ink, 2);
      stroke(sun1,2); stroke(sun2,2); stroke(sun3,2);
      stroke(sun4,2); stroke(sun5,2); stroke(sun6,2); break;
    case Icon::Music:
      stroke(note,4); iconRing(root, 5, 15, 7, ink);
      iconRing(root,14,13,7,ink); break;
    case Icon::Stocks:
    case Icon::TrendUp: stroke(trendUp,4); stroke(trendTipUp,3); break;
    case Icon::TrendDown: stroke(trendDown,4); stroke(trendTipDown,3); break;
    case Icon::Settings:
      stroke(sliderA,2); stroke(sliderB,2); stroke(sliderC,2);
      iconRing(root,7,3,6,ink); iconRing(root,14,9,6,ink);
      iconRing(root,5,15,6,ink); break;
    case Icon::Screen:
      stroke(screen,5); stroke(screenStand,4); break;
    case Icon::Ride:
      stroke(dash,5); stroke(needle,2); iconRing(root,9,14,6,ink); break;
    case Icon::Link:
      stroke(link1,4); stroke(link2,4); break;
    case Icon::Photo:
      stroke(photoBox,5); stroke(photoHill,5); iconRing(root,6,7,5,ink); break;
    case Icon::ChevronLeft: stroke(left,3); break;
    case Icon::ChevronRight: stroke(right,3); break;
    case Icon::ChevronDown: stroke(down,3); break;
    case Icon::Battery:
      stroke(battery,5); stroke(batteryTip,4); stroke(batteryFill,5); break;
  }
  return root;
}
} // namespace MotoNavUI
