#pragma once
#include "VisualLanguage.h"
// Retained LVGL ownership: an unchanged phone bitmap survives other widgets'
// redraws. Buffer allocation is once per view, never in the render loop.
namespace MotoNavUI {
struct PhoneBitmap {
  lv_obj_t *canvas = nullptr;
  lv_color_t *pixels = nullptr;
  int w=0, h=0, lastW=-1, lastH=-1;
  uint32_t revision=UINT32_MAX;
  void create(lv_obj_t *parent, int x, int y, int width, int height) {
    w=width; h=height;
    pixels=(lv_color_t *)ps_malloc(w*h*sizeof(lv_color_t));
    if (!pixels) return;
    canvas=lv_canvas_create(parent);
    lv_canvas_set_buffer(canvas,pixels,w,h,LV_IMG_CF_TRUE_COLOR);
    lv_obj_set_pos(canvas,x,y); noInteraction(canvas);
    lv_canvas_fill_bg(canvas,color(Background),LV_OPA_COVER);
  }
  void update(int slot, uint32_t background=Background, uint32_t ink=Text) {
    if (!canvas) return;
    const TextImg &im=imgs[slot];
    if (revision==im.revision && lastW==im.showW && lastH==im.showH) return;
    revision=im.revision; lastW=im.showW; lastH=im.showH;
    const lv_color_t bg=color(background), fg=color(ink);
    for(int i=0;i<w*h;++i) pixels[i]=bg;
    if(im.show && im.showW && im.showH) {
      const float scale=fminf(1.0f,fminf(float(w)/im.showW,float(h)/im.showH));
      const int dw=max(1,int(im.showW*scale)), dh=max(1,int(im.showH*scale));
      for(int y=0;y<dh;++y) for(int x=0;x<dw;++x) {
        uint8_t a=imgNib(im,(im.showW+1)/2,int(x/scale),int(y/scale));
        pixels[(y+(h-dh)/2)*w+x+(w-dw)/2]=lv_color_mix(fg,bg,a*17);
      }
    }
    lv_obj_invalidate(canvas);
  }
  void visible(bool show) {
    if(!canvas) return;
    const bool hidden=lv_obj_has_flag(canvas,LV_OBJ_FLAG_HIDDEN);
    if(hidden==!show) return;
    if(show) lv_obj_clear_flag(canvas,LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(canvas,LV_OBJ_FLAG_HIDDEN);
  }
};
}
