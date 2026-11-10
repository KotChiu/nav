#pragma once
// The genuine turn distance uses Montserrat instead of the old pixel font.
// GFX still draws the exact same distance underneath for fail-visible fallback.
// This card covers only the right-hand numeric zone of the existing turn HUD.
#include "VisualLanguage.h"
#include "NavChromePolicy.h"
#include <cstring>

namespace MotoNavUI {
struct NavDistanceView {
  lv_obj_t *root=nullptr;
  lv_obj_t *caption=nullptr;
  lv_obj_t *value=nullptr;
  bool visible=false;
  bool near=false;
  bool compact=false;
};
static NavDistanceView navDistance;

static void navDistanceCreate(lv_obj_t *screen) {
  const NavChromeRect &r=NavDistanceCard;
  navDistance.root=panel(screen,r.x,r.y,r.w,r.h,17,Surface);
  lv_obj_set_style_border_width(navDistance.root,0,0);
  navDistance.caption=label(navDistance.root,"DISTANCE",
                              &lv_font_montserrat_16,Muted);
  lv_obj_align(navDistance.caption,LV_ALIGN_TOP_MID,0,3);
  navDistance.value=label(navDistance.root,"--",
                            &lv_font_montserrat_40,Text);
  lv_obj_set_pos(navDistance.value,2,25);
  lv_obj_set_size(navDistance.value,r.w-4,45);
  lv_label_set_long_mode(navDistance.value,LV_LABEL_LONG_CLIP);
  lv_obj_set_style_text_align(navDistance.value,LV_TEXT_ALIGN_CENTER,0);
  lv_obj_add_flag(navDistance.root,LV_OBJ_FLAG_HIDDEN);
}

static void navDistanceUpdate(bool visible,const char *distance,bool isNear) {
  if (!navDistance.root) return;
  if (visible!=navDistance.visible) {
    navDistance.visible=visible;
    if (visible) lv_obj_clear_flag(navDistance.root,LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(navDistance.root,LV_OBJ_FLAG_HIDDEN);
  }
  if (!visible) return;
  const char *shown=distance ? distance : "--";
  const bool compact=std::strlen(shown)>=5;
  if (compact!=navDistance.compact) {
    navDistance.compact=compact;
    lv_obj_set_style_text_font(navDistance.value,
       compact ? &lv_font_montserrat_28 : &lv_font_montserrat_40,0);
  }
  if (std::strcmp(lv_label_get_text(navDistance.value),shown)!=0)
    lv_label_set_text(navDistance.value,shown);
  if (isNear!=navDistance.near) {
    navDistance.near=isNear;
    lv_obj_set_style_text_color(navDistance.value,
                                color(isNear ? Warm : Text),0);
  }
}
} // namespace MotoNavUI
