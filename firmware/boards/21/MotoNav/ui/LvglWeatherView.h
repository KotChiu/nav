#pragma once
// Shared LVGL 8.3 weather summary: works on ESP32 and SDL without Arduino.
// The phone remains the data source; Chinese bitmap weather extensions and
// specialized alert pages still use their existing device rendering for now.
#include <lvgl.h>
#include <cstdio>
#include <cstring>

struct MotoNavWeatherModel {
  bool available = false;
  bool connected = false;
  bool hourly = false;
  int icon = 0;
  int temperature = 0;
  int forecastCount = 0;
  int forecastHourOrWeek[6] = {};
  int forecastIcon[6] = {};
  int forecastHigh[6] = {};
  int forecastLowOrRain[6] = {};
  int aqi = -1;
  int alertCount = 0;
  const char *updated = "";
};

struct MotoNavWeatherView {
  lv_obj_t *root = nullptr;
  lv_obj_t *condition = nullptr;
  lv_obj_t *temperature = nullptr;
  lv_obj_t *summary = nullptr;
  lv_obj_t *forecastTitle = nullptr;
  lv_obj_t *forecastCell[6] = {};
  lv_obj_t *forecastTime[6] = {};
  lv_obj_t *forecastValue[6] = {};
  lv_obj_t *forecastIcon[6] = {};
  lv_obj_t *footer = nullptr;
};

static inline void motonavWeatherHide(lv_obj_t *obj, bool hidden) {
  if (!obj || lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN) == hidden) return;
  if (hidden) lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

static inline void motonavWeatherLabelText(lv_obj_t *obj, const char *text) {
  if (!obj) return;
  const char *old = lv_label_get_text(obj);
  if (!old || strcmp(old, text ? text : "") != 0)
    lv_label_set_text(obj, text ? text : "");
}

static inline lv_obj_t *motonavWeatherLabel(lv_obj_t *parent, int x, int y,
                                            int width, const lv_font_t *font,
                                            uint32_t color) {
  lv_obj_t *obj = lv_label_create(parent);
  lv_obj_set_pos(obj, x, y);
  lv_obj_set_width(obj, width);
  lv_label_set_long_mode(obj, LV_LABEL_LONG_CLIP);
  lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(obj, font, 0);
  lv_obj_set_style_text_color(obj, lv_color_hex(color), 0);
  return obj;
}

static inline const char *motonavWeatherCondition(int icon) {
  switch (icon) {
    case 0: case 10: return "CLEAR";
    case 1: case 11: return "PARTLY CLOUDY";
    case 2: return "CLOUDY";
    case 3: return "RAIN";
    case 4: return "HEAVY RAIN";
    case 5: return "THUNDER";
    case 6: return "SNOW";
    case 7: return "HAZE";
    case 8: return "SLEET";
    case 9: return "DUST";
    default: return "WEATHER";
  }
}

static inline void motonavWeatherViewCreate(MotoNavWeatherView &view, lv_obj_t *screen) {
  view.root = lv_obj_create(screen);
  lv_obj_remove_style_all(view.root);
  lv_obj_set_pos(view.root, 0, 0);
  lv_obj_set_size(view.root, 466, 466);
  lv_obj_set_style_bg_opa(view.root, LV_OPA_TRANSP, 0);
  lv_obj_clear_flag(view.root, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

  view.condition = motonavWeatherLabel(view.root, 64, 105, 338,
                                       &lv_font_montserrat_20, 0x25DEEF);
  view.temperature = motonavWeatherLabel(view.root, 66, 140, 334,
                                         &lv_font_montserrat_40, 0xFFFFFF);
  view.summary = motonavWeatherLabel(view.root, 68, 203, 330,
                                     &lv_font_montserrat_16, 0xA7B8C8);
  view.forecastTitle = motonavWeatherLabel(view.root, 50, 267, 366,
                                           &lv_font_montserrat_16, 0x25DEEF);

  for (int i = 0; i < 6; ++i) {
    view.forecastCell[i] = lv_obj_create(view.root);
    lv_obj_remove_style_all(view.forecastCell[i]);
    lv_obj_set_size(view.forecastCell[i], 58, 83);
    lv_obj_set_style_bg_opa(view.forecastCell[i], LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(view.forecastCell[i], lv_color_hex(0x0A151E), 0);
    lv_obj_set_style_radius(view.forecastCell[i], 8, 0);
    lv_obj_clear_flag(view.forecastCell[i], LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    view.forecastTime[i] = motonavWeatherLabel(view.forecastCell[i], 1, 8, 56,
                                                &lv_font_montserrat_14, 0x9DAFBF);
    view.forecastValue[i] = motonavWeatherLabel(view.forecastCell[i], 0, 39, 58,
                                                 &lv_font_montserrat_16, 0xFFFFFF);
    view.forecastIcon[i] = motonavWeatherLabel(view.forecastCell[i], 0, 62, 58,
                                                &lv_font_montserrat_14, 0x38C7D8);
  }
  view.footer = motonavWeatherLabel(view.root, 47, 398, 372,
                                    &lv_font_montserrat_14, 0x93A5B6);
  lv_obj_move_background(view.root);
  motonavWeatherHide(view.root, true);
}

static inline void motonavWeatherViewVisible(MotoNavWeatherView &view, bool visible) {
  if (!view.root) return;
  motonavWeatherHide(view.root, !visible);
}

static inline void motonavWeatherViewUpdate(MotoNavWeatherView &view,
                                             const MotoNavWeatherModel &model) {
  if (!view.root) return;
  char buffer[96];
  if (!model.available) {
    motonavWeatherLabelText(view.condition, "WEATHER");
    motonavWeatherLabelText(view.temperature, "--");
    motonavWeatherLabelText(view.summary,
                             model.connected ? "LOADING FROM PHONE" : "PHONE OFFLINE");
    motonavWeatherLabelText(view.forecastTitle, "");
    motonavWeatherLabelText(view.footer, "WAITING FOR WEATHER");
  } else {
    motonavWeatherLabelText(view.condition, motonavWeatherCondition(model.icon));
    snprintf(buffer, sizeof(buffer), "%d\xC2\xB0", model.temperature);
    motonavWeatherLabelText(view.temperature, buffer);
    motonavWeatherLabelText(view.summary,
                             !model.connected ? "PHONE OFFLINE - LAST DATA" :
                             (model.hourly ? "HOURLY FORECAST" : "DAILY FORECAST"));
    motonavWeatherLabelText(view.forecastTitle,
                             model.hourly ? "NEXT HOURS" : "NEXT DAYS");
    snprintf(buffer, sizeof(buffer), "AQI %s  |  ALERTS %d  |  %s",
             model.aqi < 0 ? "--" : "", model.alertCount < 0 ? 0 : model.alertCount,
             model.updated ? model.updated : "");
    // Numeric AQI is optional and cannot be inferred from missing phone data.
    if (model.aqi >= 0)
      snprintf(buffer, sizeof(buffer), "AQI %d  |  ALERTS %d  |  %s",
               model.aqi, model.alertCount < 0 ? 0 : model.alertCount,
               model.updated ? model.updated : "");
    motonavWeatherLabelText(view.footer, buffer);
  }
  const int count = model.available ?
    (model.forecastCount < 0 ? 0 : model.forecastCount > 6 ? 6 : model.forecastCount) : 0;
  const int start = (466 - count * 62 + 4) / 2;
  for (int i = 0; i < 6; ++i) {
    const bool needed = i < count;
    if (!needed) {
      motonavWeatherHide(view.forecastCell[i], true);
      continue;
    }
    motonavWeatherHide(view.forecastCell[i], false);
    if (lv_obj_get_x(view.forecastCell[i]) != start + i * 62 ||
        lv_obj_get_y(view.forecastCell[i]) != 295)
      lv_obj_set_pos(view.forecastCell[i], start + i * 62, 295);
    if (model.hourly) {
      snprintf(buffer, sizeof(buffer), "%02d:00", model.forecastHourOrWeek[i] % 24);
      motonavWeatherLabelText(view.forecastTime[i], buffer);
      snprintf(buffer, sizeof(buffer), "%d\xC2\xB0", model.forecastHigh[i]);
    } else {
      const char *days[] = {"", "MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};
      int week = model.forecastHourOrWeek[i];
      motonavWeatherLabelText(view.forecastTime[i],
                              i == 0 ? "TODAY" : (week >= 1 && week <= 7 ? days[week] : "--"));
      snprintf(buffer, sizeof(buffer), "%d/%d", model.forecastHigh[i],
               model.forecastLowOrRain[i]);
    }
    motonavWeatherLabelText(view.forecastValue[i], buffer);
    if (model.hourly) {
      if (model.forecastLowOrRain[i] >= 10)
        snprintf(buffer, sizeof(buffer), "%d%% RAIN", model.forecastLowOrRain[i]);
      else snprintf(buffer, sizeof(buffer), "%s", motonavWeatherCondition(model.forecastIcon[i]));
    } else snprintf(buffer, sizeof(buffer), "%s", motonavWeatherCondition(model.forecastIcon[i]));
    // Compact condition words must fit in 58 px at 14 pt.
    if (strstr(buffer, "CLOUD")) strcpy(buffer, "CLOUD");
    else if (strstr(buffer, "RAIN") && !strchr(buffer, '%')) strcpy(buffer, "RAIN");
    else if (strstr(buffer, "CLEAR")) strcpy(buffer, "SUN");
    else if (strlen(buffer) > 9) strcpy(buffer, "OTHER");
    motonavWeatherLabelText(view.forecastIcon[i], buffer);
  }
}
