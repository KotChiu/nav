#pragma once
// Electronic badge image view shared by the 2.1 ESP32 and the LVGL simulator.
// The underlying 480x480 LCD and its round physical mask are board-specific.
#include <lvgl.h>
#include <cstdint>
#include "BadgePacket.h"

struct MotoNavBadgeView {
  lv_obj_t *root = nullptr;
  lv_obj_t *picture = nullptr;
  lv_obj_t *placeholder = nullptr;
  lv_img_dsc_t image = {};
  uint32_t revision = UINT32_MAX;
};

static inline void motonavBadgeViewCreate(MotoNavBadgeView &view, lv_obj_t *parent) {
  view.root = lv_obj_create(parent);
  lv_obj_remove_style_all(view.root);
  lv_obj_set_size(view.root, 466, 466);
  lv_obj_set_pos(view.root, 0, 0);
  lv_obj_set_style_bg_color(view.root, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(view.root, LV_OPA_COVER, 0);
  lv_obj_clear_flag(view.root, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  view.picture = lv_img_create(view.root);
  lv_obj_clear_flag(view.picture, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_pos(view.picture, 113, 113); // 240 px image pivot at the center (233,233)
  lv_img_set_pivot(view.picture, 120, 120);
  lv_img_set_zoom(view.picture, 497); // 240 * 497/256 ~= 466 logical pixels
  view.placeholder = lv_label_create(view.root);
  lv_label_set_text(view.placeholder, "E-BADGE\n\nCHOOSE A PHOTO\nIN THE MOTO NAV APP");
  lv_obj_set_style_text_align(view.placeholder, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(view.placeholder, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(view.placeholder, lv_color_hex(0x25DCE9), 0);
  lv_obj_center(view.placeholder);
  lv_obj_add_flag(view.root, LV_OBJ_FLAG_HIDDEN);
}

static inline void motonavBadgeViewSet(MotoNavBadgeView &view,
                                       const uint16_t *pixels,
                                       uint32_t revision) {
  if (!view.root) return;
  const bool available = pixels != nullptr;
  if (available && (view.revision != revision || view.image.data !=
                    reinterpret_cast<const uint8_t *>(pixels))) {
    lv_img_cache_invalidate_src(&view.image);
    view.image.header.always_zero = 0;
    view.image.header.cf = LV_IMG_CF_TRUE_COLOR;
    view.image.header.w = MotoNavBadge::SIZE;
    view.image.header.h = MotoNavBadge::SIZE;
    view.image.data_size = MotoNavBadge::PIXEL_BYTES;
    view.image.data = reinterpret_cast<const uint8_t *>(pixels);
    lv_img_set_src(view.picture, &view.image);
    lv_obj_invalidate(view.picture);
    view.revision = revision;
  }
  if (available) {
    lv_obj_clear_flag(view.picture, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(view.placeholder, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(view.picture, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(view.placeholder, LV_OBJ_FLAG_HIDDEN);
  }
}
static inline void motonavBadgeViewVisible(MotoNavBadgeView &view, bool visible) {
  if (!view.root) return;
  const bool hidden = lv_obj_has_flag(view.root, LV_OBJ_FLAG_HIDDEN);
  if (visible && hidden) {
    lv_obj_move_foreground(view.root);
    lv_obj_clear_flag(view.root, LV_OBJ_FLAG_HIDDEN);
  } else if (!visible && !hidden) {
    lv_obj_add_flag(view.root, LV_OBJ_FLAG_HIDDEN);
  }
}
