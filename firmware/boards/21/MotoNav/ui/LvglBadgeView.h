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
  lv_obj_t *storage = nullptr;
  const char *storageText = nullptr;
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
  view.placeholder = lv_obj_create(view.root);
  lv_obj_remove_style_all(view.placeholder);
  lv_obj_set_size(view.placeholder, 416, 416);
  lv_obj_center(view.placeholder);
  lv_obj_clear_flag(view.placeholder, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(view.placeholder, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(view.placeholder, lv_color_hex(0x071119), 0);
  lv_obj_set_style_bg_grad_color(view.placeholder, lv_color_hex(0x120C19), 0);
  lv_obj_set_style_bg_grad_dir(view.placeholder, LV_GRAD_DIR_VER, 0);
  lv_obj_set_style_bg_opa(view.placeholder, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(view.placeholder, lv_color_hex(0x21414A), 0);
  lv_obj_set_style_border_width(view.placeholder, 1, 0);

  auto label = [&](lv_obj_t *parent, const char *text, int y,
                   const lv_font_t *font, uint32_t color) {
    lv_obj_t *obj = lv_label_create(parent);
    lv_label_set_text_static(obj, text);
    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(obj, font, 0);
    lv_obj_set_style_text_color(obj, lv_color_hex(color), 0);
    lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, y);
    return obj;
  };

  label(view.placeholder, "MOTONAV  /  PHOTO MODE", 65, &lv_font_montserrat_16, 0x76BEC7);
  lv_obj_t *icon = lv_obj_create(view.placeholder);
  lv_obj_remove_style_all(icon);
  lv_obj_set_size(icon, 72, 72);
  lv_obj_set_style_radius(icon, 20, 0);
  lv_obj_set_style_bg_color(icon, lv_color_hex(0x112831), 0);
  lv_obj_set_style_bg_opa(icon, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(icon, lv_color_hex(0x25515B), 0);
  lv_obj_set_style_border_width(icon, 1, 0);
  lv_obj_clear_flag(icon, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 102);
  label(icon, LV_SYMBOL_IMAGE, 22, &lv_font_montserrat_28, 0x5CE4EC);
  label(view.placeholder, "PHOTO BADGE", 196, &lv_font_montserrat_28, 0xEDF7FA);
  label(view.placeholder, "Choose a photo in the\nMotoNav app", 240, &lv_font_montserrat_20, 0xA8BAC6);
  view.storage = label(view.placeholder, "TEMPORARY DISPLAY", 315, &lv_font_montserrat_16, 0xF28AB6);
  lv_obj_set_style_bg_color(view.storage, lv_color_hex(0x241525), 0);
  lv_obj_set_style_bg_opa(view.storage, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(view.storage, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_pad_hor(view.storage, 12, 0);
  lv_obj_set_style_pad_ver(view.storage, 5, 0);
  lv_obj_add_flag(view.root, LV_OBJ_FLAG_HIDDEN);
}

static inline void motonavBadgeViewSet(MotoNavBadgeView &view,
                                       const uint16_t *pixels,
                                       uint32_t revision,
                                       const char *storageText) {
  if (!view.root) return;
  const bool available = pixels != nullptr;
  if (view.storageText != storageText) {
    view.storageText = storageText;
    lv_label_set_text_static(view.storage, storageText);
    lv_obj_align(view.storage, LV_ALIGN_TOP_MID, 0, 315);
  }
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
