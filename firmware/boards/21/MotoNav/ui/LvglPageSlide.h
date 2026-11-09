#pragma once
// Native LVGL 8.3 image-layer page transition using the *existing* two
// PSRAM page snapshots. No third framebuffer and no Canvas pixel recomposition.
// It is enabled only when both screens are LVGL-driven, until physical board
// validation confirms mixed Canvas transitions can use this path as well.
#include <lvgl.h>
#include <cstdint>
#include "PageSlideGeometry.h"

struct MotoNavLvglPageSlide {
  lv_obj_t *root = nullptr;
  lv_obj_t *oldFrame = nullptr;
  lv_obj_t *newFrame = nullptr;
  lv_img_dsc_t oldSource = {};
  lv_img_dsc_t newSource = {};
  bool active = false;
  int width = 466;
  int height = 466;
};

static inline bool motonavLvglSlideCreate(
    MotoNavLvglPageSlide &slide, lv_obj_t *parent, int width, int height) {
  if (!parent || width <= 0 || height <= 0) return false;
  slide.width = width;
  slide.height = height;
  slide.root = lv_obj_create(parent);
  if (!slide.root) return false;
  lv_obj_remove_style_all(slide.root);
  lv_obj_set_pos(slide.root, 0, 0);
  lv_obj_set_size(slide.root, width, height);
  lv_obj_set_style_bg_opa(slide.root, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(slide.root, lv_color_black(), 0);
  lv_obj_clear_flag(slide.root, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  slide.oldFrame = lv_img_create(slide.root);
  slide.newFrame = lv_img_create(slide.root);
  if (!slide.oldFrame || !slide.newFrame) {
    lv_obj_del(slide.root);
    slide.root = nullptr;
    slide.oldFrame = nullptr;
    slide.newFrame = nullptr;
    return false;
  }
  lv_obj_clear_flag(slide.oldFrame, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(slide.newFrame, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(slide.root, LV_OBJ_FLAG_HIDDEN);
  return true;
}

static inline bool motonavLvglSlideBegin(
    MotoNavLvglPageSlide &slide, const uint16_t *oldPixels,
    const uint16_t *newPixels) {
  if (!slide.root || !oldPixels || !newPixels) return false;
  const uint32_t bytes = uint32_t(slide.width) * slide.height * sizeof(uint16_t);
  slide.oldSource.header.always_zero = 0;
  slide.oldSource.header.cf = LV_IMG_CF_TRUE_COLOR;
  slide.oldSource.header.w = slide.width;
  slide.oldSource.header.h = slide.height;
  slide.oldSource.data_size = bytes;
  slide.oldSource.data = reinterpret_cast<const uint8_t *>(oldPixels);
  slide.newSource.header.always_zero = 0;
  slide.newSource.header.cf = LV_IMG_CF_TRUE_COLOR;
  slide.newSource.header.w = slide.width;
  slide.newSource.header.h = slide.height;
  slide.newSource.data_size = bytes;
  slide.newSource.data = reinterpret_cast<const uint8_t *>(newPixels);
  lv_img_cache_invalidate_src(&slide.oldSource);
  lv_img_cache_invalidate_src(&slide.newSource);
  lv_img_set_src(slide.oldFrame, &slide.oldSource);
  lv_img_set_src(slide.newFrame, &slide.newSource);
  lv_obj_move_foreground(slide.root);
  lv_obj_clear_flag(slide.root, LV_OBJ_FLAG_HIDDEN);
  slide.active = true;
  return true;
}

static inline void motonavLvglSlideMove(
    MotoNavLvglPageSlide &slide, int amount, bool horizontal, int direction) {
  if (!slide.active) return;
  const auto p = motonavSlidePositions(
      amount, horizontal, direction, slide.width, slide.height);
  if (lv_obj_get_x(slide.oldFrame) != p.oldX || lv_obj_get_y(slide.oldFrame) != p.oldY)
    lv_obj_set_pos(slide.oldFrame, p.oldX, p.oldY);
  if (lv_obj_get_x(slide.newFrame) != p.newX || lv_obj_get_y(slide.newFrame) != p.newY)
    lv_obj_set_pos(slide.newFrame, p.newX, p.newY);
}

static inline void motonavLvglSlideEnd(MotoNavLvglPageSlide &slide) {
  if (!slide.active) return;
  slide.active = false;
  lv_obj_add_flag(slide.root, LV_OBJ_FLAG_HIDDEN);
}
