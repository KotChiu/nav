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

static inline void motonavMusicHide(lv_obj_t *obj, bool hidden) {
  if (!obj || lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN) == hidden) return;
  if (hidden) lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

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
  motonavMusicHide(view.root, true);
}

static inline void motonavMusicStatusVisible(MotoNavMusicStatusView &view, bool visible) {
  if (!view.root) return;
  motonavMusicHide(view.root, !visible);
}

static inline void motonavMusicStatusUpdate(MotoNavMusicStatusView &view,
                                             bool coverVisible, bool lyricsVisible,
                                             const char *state, const char *footer,
                                             bool error) {
  if (!view.root) return;
  motonavMusicHide(view.heading, coverVisible);
  const int statusY = lyricsVisible ? 272 : coverVisible ? 276 : 257;
  if (lv_obj_get_y(view.status) != statusY) lv_obj_set_y(view.status, statusY);
  const lv_color_t statusColor = lv_color_hex(error ? 0xF815A8 : 0x9EB0BF);
  if (lv_color_to16(lv_obj_get_style_text_color(view.status, 0)) != lv_color_to16(statusColor))
    lv_obj_set_style_text_color(view.status, statusColor, 0);
  motonavMusicSetText(view.status, state);
  const int footerY = coverVisible ? 410 : 402;
  if (lv_obj_get_y(view.footer) != footerY) lv_obj_set_y(view.footer, footerY);
  motonavMusicSetText(view.footer, footer);
  motonavMusicHide(view.footer, !footer || !footer[0]);
}
