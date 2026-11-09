#pragma once
// Main weather page is a quiet instrument card. QWeather bitmap DETAIL
// screens stay on the existing canvas to preserve the phone's Chinese glyphs.
#include "VisualLanguage.h"

namespace MotoNavUI {
struct WeatherView {
  lv_obj_t *root = nullptr, *temp = nullptr, *condition = nullptr;
  lv_obj_t *availability = nullptr, *orb = nullptr;
  lv_obj_t *forecastDay[4] = {};
  lv_obj_t *forecastTemp[4] = {};
  lv_obj_t *footer = nullptr;
  bool wasVisible = false;
  int lastSignature = -1;
};
static WeatherView weather;

static const char *conditionName(int icon) {
  switch (icon) {
    case 0: case 10: return "CLEAR SKIES";
    case 1: case 11: return "PARTLY CLOUDY";
    case 2: return "OVERCAST";
    case 3: return "LIGHT RAIN";
    case 4: return "HEAVY RAIN";
    case 5: return "THUNDERSTORM";
    case 6: return "SNOW";
    case 7: return "HAZE";
    case 8: return "SLEET";
    case 9: return "DUST";
    default: return "WEATHER";
  }
}
static void weatherCreate(lv_obj_t *screen) {
  weather.root = lv_obj_create(screen);
  lv_obj_remove_style_all(weather.root);
  lv_obj_set_size(weather.root, Width, Width);
  noInteraction(weather.root);
  lv_obj_add_flag(weather.root, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t *head = label(weather.root, "WEATHER  /  NOW",
                          &lv_font_montserrat_20, Mint);
  lv_obj_align(head, LV_ALIGN_TOP_MID, 0, 57);
  lv_obj_set_style_text_letter_space(head, 2, 0);
  weather.availability = label(weather.root, "FROM YOUR PHONE",
                          &lv_font_montserrat_16, Muted);
  lv_obj_align(weather.availability, LV_ALIGN_TOP_MID, 0, 90);

  lv_obj_t *hero = panel(weather.root, 70, 128, 326, 144, 27);
  weather.orb = lv_obj_create(hero);
  lv_obj_set_pos(weather.orb, 34, 39);
  lv_obj_set_size(weather.orb, 66, 66);
  lv_obj_set_style_radius(weather.orb, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(weather.orb, 8, 0);
  lv_obj_set_style_border_color(weather.orb, color(Mint), 0);
  lv_obj_set_style_bg_color(weather.orb, color(SurfaceRaised), 0);
  lv_obj_set_style_bg_opa(weather.orb, LV_OPA_COVER, 0);
  noInteraction(weather.orb);

  weather.temp = label(hero, "--°", &lv_font_montserrat_48, Text);
  lv_obj_set_pos(weather.temp, 119, 22);
  weather.condition = label(hero, "CONNECT YOUR PHONE",
                             &lv_font_montserrat_16, Muted);
  lv_obj_set_pos(weather.condition, 119, 98);

  lv_obj_t *forecast = panel(weather.root, 70, 288, 326, 100, 24,
                              SurfaceRaised);
  for (int i = 0; i < 4; ++i) {
    lv_obj_t *day = label(forecast, "--", &lv_font_montserrat_16, Muted);
    lv_obj_set_pos(day, 15 + i * 78, 18);
    lv_obj_set_width(day, 70);
    lv_obj_set_style_text_align(day, LV_TEXT_ALIGN_CENTER, 0);
    weather.forecastDay[i] = day;
    lv_obj_t *temp = label(forecast, "--", &lv_font_montserrat_20, Text);
    lv_obj_set_pos(temp, 15 + i * 78, 51);
    lv_obj_set_width(temp, 70);
    lv_obj_set_style_text_align(temp, LV_TEXT_ALIGN_CENTER, 0);
    weather.forecastTemp[i] = temp;
  }
  weather.footer = label(weather.root, "TAP FOR RAIN AND AIR QUALITY",
                         &lv_font_montserrat_16, Faint);
  lv_obj_align(weather.footer, LV_ALIGN_TOP_MID, 0, 407);
}
static void weatherUpdate(bool visible) {
  if (!weather.root) return;
  if (!visible) {
    if (weather.wasVisible) lv_obj_add_flag(weather.root, LV_OBJ_FLAG_HIDDEN);
    weather.wasVisible = false;
    return;
  }
  if (!weather.wasVisible) {
    lv_obj_clear_flag(weather.root, LV_OBJ_FLAG_HIDDEN);
    weather.lastSignature = -1;
    weather.wasVisible = true;
  }
  // Most fields change only when the app sends new weather. Avoid per-frame
  // LVGL layout/repaints on the continuous map render loop.
  int signature = (wxHave ? 1 : 0) ^ (wxIcon << 2) ^ (wxTemp << 7) ^
                  ((int)wxN << 16) ^ (wxHourly ? 0x100000 : 0);
  for (int i = 0; i < wxN && i < 4; ++i) signature ^= (wxFh[i] + wxFl[i] * 7 +
    wxWk[i] * 31 + wxFi[i] * 79) << (i & 3);
  if (signature == weather.lastSignature) return;
  weather.lastSignature = signature;
  char str[30];
  if (wxHave) snprintf(str, sizeof(str), "%d°", wxTemp);
  else strcpy(str, "--°");
  lv_label_set_text(weather.temp, str);
  lv_label_set_text(weather.condition, wxHave ? conditionName(wxIcon) :
                    "CONNECT YOUR PHONE");
  lv_label_set_text(weather.availability, wxHave ? "LIVE CONDITIONS" :
                    "WAITING FOR PHONE");
  lv_obj_set_style_border_color(weather.orb,
     color(wxIcon >= 3 && wxIcon <= 9 ? 0x7DAAFF : Mint), 0);
  for (int i = 0; i < 4; ++i) {
    if (i >= wxN || !wxHave) {
      lv_label_set_text(weather.forecastDay[i], "--");
      lv_label_set_text(weather.forecastTemp[i], "--");
      continue;
    }
    if (wxHourly) snprintf(str, sizeof(str), "%02d H", wxWk[i]);
    else if (i == 0) strcpy(str, "TODAY");
    else snprintf(str, sizeof(str), "DAY %d", i + 1);
    lv_label_set_text(weather.forecastDay[i], str);
    if (wxHourly) snprintf(str, sizeof(str), "%d°", wxFh[i]);
    else snprintf(str, sizeof(str), "%d/%d", wxFh[i], wxFl[i]);
    lv_label_set_text(weather.forecastTemp[i], str);
  }
  lv_label_set_text(weather.footer, wxExtra ?
                    "TAP TO VIEW DETAILS" : "WEATHER SYNCED FROM PHONE");
}
}
