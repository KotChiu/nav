#pragma once
// Map speed display in the same Montserrat / graphite language as the Home UI.
// The original GFX speed remains underneath as a fail-visible fallback:
// if this LVGL layer cannot paint, navigation does not lose the speed readout.
#include "VisualLanguage.h"
#include "NavChromePolicy.h"
#include <cstdio>
#include <cstring>

namespace MotoNavUI {
struct NavSpeedView {
  lv_obj_t *root=nullptr;
  lv_obj_t *value=nullptr;
  lv_obj_t *unit=nullptr;
  int lastSpeed=-2;
  bool lastAlert=false;
  bool lastCamera=false;
  bool lastVisible=false;
};
static NavSpeedView navSpeed;

static void navSpeedCreate(lv_obj_t *screen) {
  const NavChromeRect &r=NavSpeedCard;
  navSpeed.root=panel(screen,r.x,r.y,r.w,r.h,21,Surface);
  lv_obj_set_style_border_color(navSpeed.root,color(Line),0);
  navSpeed.value=label(navSpeed.root,"--",&lv_font_montserrat_40,Text);
  lv_obj_set_size(navSpeed.value,112,47);
  lv_obj_set_pos(navSpeed.value,2,1);
  lv_label_set_long_mode(navSpeed.value,LV_LABEL_LONG_CLIP);
  lv_obj_set_style_text_align(navSpeed.value,LV_TEXT_ALIGN_CENTER,0);
  navSpeed.unit=label(navSpeed.root,"KM/H",&lv_font_montserrat_16,Muted);
  lv_obj_align(navSpeed.unit,LV_ALIGN_BOTTOM_MID,0,-2);
  // Small, static bottom accent balances the stronger turn-progress ring.
  lv_obj_t *tick=lv_obj_create(navSpeed.root);
  lv_obj_remove_style_all(tick);
  lv_obj_set_size(tick,19,2);
  lv_obj_align(tick,LV_ALIGN_BOTTOM_MID,0,0);
  lv_obj_set_style_bg_color(tick,color(Accent),0);
  lv_obj_set_style_bg_opa(tick,LV_OPA_COVER,0);
  noInteraction(tick);
  lv_obj_add_flag(navSpeed.root,LV_OBJ_FLAG_HIDDEN);
}

static void navSpeedUpdate(bool visible,int speed,bool camera,bool alert) {
  if (!navSpeed.root) return;
  if (visible != navSpeed.lastVisible) {
    navSpeed.lastVisible=visible;
    if (visible) lv_obj_clear_flag(navSpeed.root,LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(navSpeed.root,LV_OBJ_FLAG_HIDDEN);
  }
  if (!visible) return;
  if (camera!=navSpeed.lastCamera) {
    navSpeed.lastCamera=camera;
    const NavChromeRect &r=camera ? NavSpeedCardWithCamera : NavSpeedCard;
    lv_obj_set_pos(navSpeed.root,r.x,r.y);
  }
  if (speed != navSpeed.lastSpeed) {
    navSpeed.lastSpeed=speed;
    char buf[12]; snprintf(buf,sizeof(buf),"%d",speed);
    if (std::strcmp(lv_label_get_text(navSpeed.value),buf)!=0)
      lv_label_set_text(navSpeed.value,buf);
  }
  if (alert!=navSpeed.lastAlert) {
    navSpeed.lastAlert=alert;
    lv_obj_set_style_text_color(navSpeed.value,
                                color(alert ? Red : Text),0);
    lv_obj_set_style_border_color(navSpeed.root,
                                  color(alert ? Red : Line),0);
  }
}
} // namespace MotoNavUI
