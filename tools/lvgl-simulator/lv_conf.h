#ifndef MOTONAV_SIM_LV_CONF_H
#define MOTONAV_SIM_LV_CONF_H
// LVGL v8.3 desktop configuration. This simulator is a different target
// from the embedded Arduino build and must not replace its lv_conf.h.
#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0
#define LV_MEM_SIZE (256U * 1024U)
#define LV_USE_LOG 0
#define LV_USE_ASSERT_MALLOC 1
#define LV_USE_ASSERT_STYLE 1
#define LV_TICK_CUSTOM 0
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_40 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14
#define LV_USE_ARC 1
#define LV_USE_SLIDER 1
#define LV_USE_LABEL 1
#define LV_USE_LINE 1
#define LV_USE_BTN 1
#endif
