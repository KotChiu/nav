#pragma once
// The former expressive-face page becomes an instrument-style ride dashboard.
// No decorative avatar, pixel sprite or automatic expression cycling.
#include "VisualLanguage.h"

namespace MotoNavUI {
struct RideView {
  lv_obj_t *root = nullptr;
  lv_obj_t *progress = nullptr;
  lv_obj_t *speed = nullptr;
  lv_obj_t *speedUnit = nullptr;
  lv_obj_t *mode = nullptr;
  lv_obj_t *status = nullptr;
  lv_obj_t *subtext = nullptr;
};
static RideView ride;

static void rideCreate(lv_obj_t *parent) {
  ride.root = lv_obj_create(parent);
  lv_obj_remove_style_all(ride.root);
  lv_obj_set_pos(ride.root, 0, 0);
  lv_obj_set_size(ride.root, Width, Width);
  noInteraction(ride.root);
  lv_obj_add_flag(ride.root, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t *eyebrow = label(ride.root, "MOTONAV  /  RIDE MODE",
                            &lv_font_montserrat_16, Accent);
  lv_obj_align(eyebrow, LV_ALIGN_TOP_MID, 0, 57);
  lv_obj_set_style_text_letter_space(eyebrow, 2, 0);

  arc(ride.root, 60, 86, 346, 30, 150, Line, 2);
  arc(ride.root, 60, 86, 346, 210, 330, Line, 2);

  ride.progress = lv_arc_create(ride.root);
  lv_obj_set_pos(ride.progress, 95, 121);
  lv_obj_set_size(ride.progress, 276, 276);
  lv_arc_set_range(ride.progress, 0, 1000);
  lv_arc_set_rotation(ride.progress, 270);
  lv_arc_set_bg_angles(ride.progress, 0, 360);
  lv_obj_set_style_arc_width(ride.progress, 8, LV_PART_MAIN);
  lv_obj_set_style_arc_color(ride.progress, color(SurfaceRaised), LV_PART_MAIN);
  lv_obj_set_style_arc_width(ride.progress, 8, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(ride.progress, color(Accent), LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(ride.progress, LV_OPA_TRANSP, LV_PART_KNOB);
  lv_arc_set_value(ride.progress, 0);
  noInteraction(ride.progress);

  ride.mode = label(ride.root, "RIDE READY", &lv_font_montserrat_20, Text);
  lv_obj_align(ride.mode, LV_ALIGN_TOP_MID, 0, 155);
  ride.speed = label(ride.root, "--", &lv_font_montserrat_48, Text);
  lv_obj_align(ride.speed, LV_ALIGN_TOP_MID, 0, 195);
  ride.speedUnit = label(ride.root, "KM / H", &lv_font_montserrat_16, Muted);
  lv_obj_align(ride.speedUnit, LV_ALIGN_TOP_MID, 0, 260);

  lv_obj_t *divider = lv_obj_create(ride.root);
  lv_obj_remove_style_all(divider);
  lv_obj_set_size(divider, 110, 1);
  lv_obj_set_style_bg_color(divider, color(Line), 0);
  lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, 0);
  lv_obj_align(divider, LV_ALIGN_TOP_MID, 0, 299);
  noInteraction(divider);

  ride.status = label(ride.root, "WAITING FOR PHONE",
                      &lv_font_montserrat_20, Warm);
  lv_obj_align(ride.status, LV_ALIGN_TOP_MID, 0, 323);
  ride.subtext = label(ride.root, "THE ROAD IS YOURS.",
                       &lv_font_montserrat_16, Muted);
  lv_obj_align(ride.subtext, LV_ALIGN_TOP_MID, 0, 366);
  lv_obj_t *foot = label(ride.root, "FOCUS  /  FLOW  /  FREEDOM",
                         &lv_font_montserrat_16, Faint);
  lv_obj_align(foot, LV_ALIGN_TOP_MID, 0, 413);
}
static void rideUpdate(bool visible) {
  if (!ride.root) return;
  if (!visible) {
    if (!lv_obj_has_flag(ride.root, LV_OBJ_FLAG_HIDDEN))
      lv_obj_add_flag(ride.root, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  if (lv_obj_has_flag(ride.root, LV_OBJ_FLAG_HIDDEN))
    lv_obj_clear_flag(ride.root, LV_OBJ_FLAG_HIDDEN);

  static int previousSpeed = -999, previousProgress = -999;
  static bool previousConnected = false, previousNav = false;
  const bool connected = bleConnected && bleAuthed;
  int speed = speedKmh >= 0 ? speedKmh : -1;
  if (speed != previousSpeed) {
    previousSpeed = speed;
    char buffer[14];
    if (speed < 0) strcpy(buffer, "--");
    else snprintf(buffer, sizeof(buffer), "%d", speed);
    lv_label_set_text(ride.speed, buffer);
    lv_obj_align(ride.speed, LV_ALIGN_TOP_MID, 0, 195);
  }
  float progress = tripOn && inNav ? tripFrac() : -1.f;
  const int currentProgress = progress < 0.f ? 0 :
    constrain((int)lroundf(progress * 1000.f), 0, 1000);
  if (currentProgress != previousProgress) {
    previousProgress = currentProgress;
    lv_arc_set_value(ride.progress, currentProgress);
  }
  if (previousConnected != connected || previousNav != inNav) {
    previousConnected = connected;
    previousNav = inNav;
    lv_label_set_text(ride.mode, inNav ? "ON THE ROAD" : "RIDE READY");
    lv_label_set_text(ride.status, connected ?
                      "PHONE CONNECTED" : "WAITING FOR PHONE");
    lv_obj_set_style_text_color(ride.status,
        color(connected ? Cool : Warm), 0);
  }
}
}
