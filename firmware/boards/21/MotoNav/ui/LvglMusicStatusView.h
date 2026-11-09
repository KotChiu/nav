#pragma once
// Shared native LVGL 8.3 playback-status UI.
// Device and desktop pass their own player state; no Arduino/BLE dependencies.
#include <lvgl.h>
#include <cstring>

struct MotoNavMusicStatusView {
  lv_obj_t *root = nullptr;
  lv_obj_t *heading = nullptr;
  lv_obj_t *status = nullptr;
  lv_obj_t *footer = nullptr;
};

static inline void motonavMusicSetText(lv_obj_t *obj, const char *text) {
  if (!obj) return;
  const char *value = text ? text : "";
  const char *old = lv_label_get_text(obj);
  if (!old || strcmp(old, value) != 0) lv_label_set_text(obj, value);
}

static inline lv_obj_t *motonavMusicText(lv_obj_t *parent, int y,
                                         const lv_font_t *font, uint32_t color) {
  lv_obj_t *label = lv_label_create(parent);
  lv_obj_set_size(label, 420, LV_SIZE_CONTENT);
  lv_obj_set_pos(label, 23, y);
  lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
  lv_obj_set_style_text_font(label, font, 0);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
  return label;
}

static inline void motonavMusicStatusCreate(MotoNavMusicStatusView &view,
                                             lv_obj_t *screen) {
  view.root = lv_obj_create(screen);
  lv_obj_remove_style_all(view.root);
  lv_obj_set_pos(view.root, 0, 0);
  lv_obj_set_size(view.root, 466, 466);
  lv_obj_set_style_bg_opa(view.root, LV_OPA_TRANSP, 0);
  lv_obj_clear_flag(view.root, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  view.heading = motonavMusicText(view.root, 59, &lv_font_montserrat_28, 0x27D7EA);
  motonavMusicSetText(view.heading, "MUSIC");
  view.status = motonavMusicText(view.root, 252, &lv_font_montserrat_16, 0x9EB0BF);
  view.footer = motonavMusicText(view.root, 403, &lv_font_montserrat_14, 0x9EB0BF);
  lv_obj_move_background(view.root);
  lv_obj_add_flag(view.root, LV_OBJ_FLAG_HIDDEN);
}

static inline void motonavMusicStatusVisible(MotoNavMusicStatusView &view, bool visible) {
  if (!view.root) return;
  bool hidden = lv_obj_has_flag(view.root, LV_OBJ_FLAG_HIDDEN);
  if (visible && hidden) lv_obj_clear_flag(view.root, LV_OBJ_FLAG_HIDDEN);
  else if (!visible && !hidden) lv_obj_add_flag(view.root, LV_OBJ_FLAG_HIDDEN);
}

static inline void motonavMusicStatusUpdate(MotoNavMusicStatusView &view,
                                             bool coverVisible, bool lyricsVisible,
                                             const char *state, const char *footer,
                                             bool error) {
  if (!view.root) return;
  if (coverVisible) lv_obj_add_flag(view.heading, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_clear_flag(view.heading, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_y(view.status, lyricsVisible ? 272 : coverVisible ? 276 : 257);
  lv_obj_set_style_text_color(view.status, lv_color_hex(error ? 0xF815A8 : 0x9EB0BF), 0);
  motonavMusicSetText(view.status, state);
  lv_obj_set_y(view.footer, coverVisible ? 410 : 402);
  motonavMusicSetText(view.footer, footer);
  if (footer && footer[0]) lv_obj_clear_flag(view.footer, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_add_flag(view.footer, LV_OBJ_FLAG_HIDDEN);
}
