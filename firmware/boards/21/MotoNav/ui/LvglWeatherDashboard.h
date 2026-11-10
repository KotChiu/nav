#pragma once
// Main weather page is a quiet instrument card. QWeather bitmap DETAIL
// screens stay on the existing canvas to preserve the phone's Chinese glyphs.
#include "VisualLanguage.h"
#include "LvglIcons.h"
#include "LvglPhoneBitmap.h"

namespace MotoNavUI {
struct WeatherView {
  lv_obj_t *root = nullptr, *temp = nullptr, *condition = nullptr;
  lv_obj_t *availability = nullptr, *orb = nullptr;
  lv_obj_t *forecastDay[4] = {};
  lv_obj_t *forecastTemp[4] = {};
  lv_obj_t *footer = nullptr, *head=nullptr, *hero=nullptr, *forecast=nullptr;
  lv_obj_t *forecastIcon[4]={};
  int iconIds[5]={-1,-1,-1,-1,-1};
  lv_obj_t *details[2]={}, *rainLine=nullptr, *air=nullptr, *rainSummary=nullptr;
  lv_point_t rainPoints[24]={};
  PhoneBitmap detailText[5];
  int lastDetail=-1;
  bool wasVisible = false;
  uint32_t lastSignature = UINT32_MAX;
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
// Distinct vector conditions; no font-dependent sun/emoji glyphs.
static void weatherGlyph(lv_obj_t *root,int id) {
  lv_obj_clean(root);
  auto disc=[&](int x,int y,int w,int h,uint32_t tint) {
    lv_obj_t *o=panel(root,x,y,w,h,LV_RADIUS_CIRCLE,tint);
    lv_obj_set_style_border_width(o,0,0);
    return o;
  };
  if(id==0 || id==1 || id==10 || id==11) {
    disc(19,5,25,25,Warm);
    if(id>=10) disc(27,1,24,24,Surface);
  }
  if(id!=0 && id!=10 && id!=7 && id!=9) {
    disc(8,22,24,20,Muted); disc(21,14,28,28,Muted); disc(38,25,19,17,Muted);
  }
  if(id==3 || id==4 || id==5 || id==8)
    for(int i=0;i<3;++i) disc(16+i*12,46,3,10,Cool);
  if(id==5) disc(31,33,5,19,Warm);
  if(id==6) for(int i=0;i<3;++i) disc(16+i*12,48,4,4,Text);
  if(id==7 || id==9) for(int i=0;i<4;++i) disc(9+(i%2)*4,12+i*10,43,3,id==9?Warm:Muted);
}
static void weatherCreate(lv_obj_t *screen) {
  weather.root = lv_obj_create(screen);
  lv_obj_remove_style_all(weather.root);
  lv_obj_set_size(weather.root, Width, Width);
  noInteraction(weather.root);
  lv_obj_add_flag(weather.root, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t *head = weather.head = label(weather.root, "WEATHER  /  NOW",
                          &lv_font_montserrat_20, Cool);
  lv_obj_align(head, LV_ALIGN_TOP_MID, 0, 57);
  lv_obj_set_style_text_letter_space(head, 2, 0);
  weather.availability = label(weather.root, "FROM YOUR PHONE",
                          &lv_font_montserrat_16, Muted);
  lv_obj_align(weather.availability, LV_ALIGN_TOP_MID, 0, 90);

  lv_obj_t *hero = weather.hero = panel(weather.root, 70, 128, 326, 144, 27);
  weather.orb=lv_obj_create(hero);
  lv_obj_remove_style_all(weather.orb);
  lv_obj_set_pos(weather.orb,27,38); lv_obj_set_size(weather.orb,64,64);
  noInteraction(weather.orb);
  weather.temp = label(hero, "--°", &lv_font_montserrat_48, Text);
  lv_obj_set_pos(weather.temp, 119, 22);
  weather.condition = label(hero, "CONNECT YOUR PHONE",
                             &lv_font_montserrat_16, Muted);
  lv_obj_set_pos(weather.condition, 119, 98);
  lv_obj_set_width(weather.condition,190);
  lv_label_set_long_mode(weather.condition,LV_LABEL_LONG_DOT);

  lv_obj_t *forecast = weather.forecast = panel(weather.root, 70, 282, 326, 118, 24,
                              SurfaceRaised);
  for (int i = 0; i < 4; ++i) {
    lv_obj_t *day = label(forecast, "--", &lv_font_montserrat_16, Muted);
    lv_obj_set_pos(day, 9 + i * 78, 10);
    lv_obj_set_width(day, 70);
    lv_obj_set_style_text_align(day, LV_TEXT_ALIGN_CENTER, 0);
    weather.forecastDay[i] = day;
    weather.forecastIcon[i]=lv_obj_create(forecast);
    lv_obj_remove_style_all(weather.forecastIcon[i]);
    lv_obj_set_pos(weather.forecastIcon[i],12+i*78,28);
    lv_obj_set_size(weather.forecastIcon[i],64,56);
    noInteraction(weather.forecastIcon[i]);
    lv_obj_t *temp = label(forecast, "--", &lv_font_montserrat_20, Text);
    lv_obj_set_pos(temp, 9 + i * 78, 85);
    lv_obj_set_width(temp, 78);
    lv_label_set_long_mode(temp,LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(temp, LV_TEXT_ALIGN_CENTER, 0);
    weather.forecastTemp[i] = temp;
  }
  for(int i=0;i<2;++i) {
    weather.details[i]=panel(weather.root,70,128,326,272,27);
    lv_obj_add_flag(weather.details[i],LV_OBJ_FLAG_HIDDEN);
  }
  weather.detailText[0].create(weather.details[0],13,14,300,46);
  weather.detailText[1].create(weather.details[0],13,177,300,36);
  weather.rainLine=lv_line_create(weather.details[0]);
  lv_obj_set_pos(weather.rainLine,25,70);
  lv_obj_set_style_line_width(weather.rainLine,3,0);
  lv_obj_set_style_line_color(weather.rainLine,color(Cool),0);
  weather.rainSummary=label(weather.details[0],"",&lv_font_montserrat_16,Muted);
  lv_obj_set_pos(weather.rainSummary,20,145);
  weather.air=label(weather.details[0],"AQI --",&lv_font_montserrat_28,Text);
  lv_obj_align(weather.air,LV_ALIGN_BOTTOM_MID,0,-15);
  for(int i=2;i<5;++i) weather.detailText[i].create(weather.details[1],13,18+(i-2)*76,300,64);
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
    weather.lastSignature = UINT32_MAX;
    weather.wasVisible = true;
  }
  // Most fields change only when the app sends new weather. Avoid per-frame
  // LVGL layout/repaints on the continuous map render loop.
  uint32_t signature = (wxHave ? 1U : 0U) ^ ((uint32_t)wxIcon << 2) ^
                       ((uint32_t)wxTemp << 7) ^ ((uint32_t)wxN << 16) ^
                       (wxHourly ? 0x100000U : 0U);
  for (int i = 0; i < wxN && i < 4; ++i) {
    const uint32_t item = (uint32_t)(wxFh[i] + wxFl[i] * 7 +
                                      wxWk[i] * 31 + wxFi[i] * 79);
    signature ^= item << (i & 3);
  }
  signature ^= uint32_t(wxDetail*131 + wxAQI*17 + wxAlerts*23 + wxLevel*43 + wxExtra*257);
  for(int i=0;i<wxRainN;++i) signature=signature*31+wxRain[i];
  for(int i=7;i<=11;++i) signature=signature*31+imgs[i].revision;
  if (signature == weather.lastSignature) return;
  weather.lastSignature = signature;
  char str[64];
  auto show=[](lv_obj_t *obj,bool visible) {
    if(visible) lv_obj_clear_flag(obj,LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(obj,LV_OBJ_FLAG_HIDDEN);
  };
  show(weather.hero,wxDetail==0); show(weather.forecast,wxDetail==0);
  show(weather.details[0],wxDetail==1); show(weather.details[1],wxDetail==2);
  lv_label_set_text(weather.head,wxDetail==0?"WEATHER / NOW":wxDetail==1?"RAIN / AIR":"WEATHER / ALERTS");
  for(int i=0;i<5;++i) weather.detailText[i].update(7+i,Surface);
  int peak=0;
  for(int i=0;i<wxRainN;++i) peak=max(peak,wxRain[i]);
  for(int i=0;i<wxRainN;++i) weather.rainPoints[i]={(lv_coord_t)(i*276/max(1,wxRainN-1)),(lv_coord_t)(64-max(0,wxRain[i])*60/max(1,peak))};
  show(weather.rainLine,wxRainN>=2);
  if(wxRainN>=2) lv_line_set_points(weather.rainLine,weather.rainPoints,wxRainN);
  if(wxRainN) snprintf(str,sizeof(str),"NEXT 2H / MAX %.2f mm",peak/100.0f);
  else snprintf(str,sizeof(str),"RAIN DATA UNAVAILABLE");
  lv_label_set_text(weather.rainSummary,str);
  if(wxAQI>=0) snprintf(str,sizeof(str),"AQI %d",wxAQI);
  else snprintf(str,sizeof(str),"AQI --");
  lv_label_set_text(weather.air,str);
  if(weather.iconIds[0]!=wxIcon) { weatherGlyph(weather.orb,wxIcon); weather.iconIds[0]=wxIcon; }

  if (wxHave) snprintf(str, sizeof(str), "%d°", wxTemp);
  else strcpy(str, "--°");
  lv_label_set_text(weather.temp, str);
  lv_label_set_text(weather.condition, wxHave ? conditionName(wxIcon) :
                    "CONNECT YOUR PHONE");
  lv_label_set_text(weather.availability, wxHave ? "LIVE CONDITIONS" :
                    "WAITING FOR PHONE");
  lv_obj_set_style_border_color(weather.orb,
     color(wxIcon >= 3 && wxIcon <= 9 ? 0x6FA4D8 : Cool), 0);
  for (int i = 0; i < 4; ++i) {
    show(weather.forecastIcon[i],i<wxN && wxHave);
    if(i<wxN && wxHave && weather.iconIds[i+1]!=wxFi[i]) {
      weatherGlyph(weather.forecastIcon[i],wxFi[i]); weather.iconIds[i+1]=wxFi[i];
    }
    if (i >= wxN || !wxHave) {
      lv_label_set_text(weather.forecastDay[i], "--");
      lv_label_set_text(weather.forecastTemp[i], "--");
      continue;
    }
    if (wxHourly) snprintf(str, sizeof(str), "%02d H", wxWk[i]);
    else if (i == 0) strcpy(str, "TODAY");
    else snprintf(str, sizeof(str), "%s", WX_WEEK[constrain(wxWk[i],1,7)]);
    lv_label_set_text(weather.forecastDay[i], str);
    if (wxHourly) snprintf(str, sizeof(str), "%d°", wxFh[i]);
    else snprintf(str, sizeof(str), "%d/%d°", wxFh[i], wxFl[i]);
    lv_label_set_text(weather.forecastTemp[i], str);
  }
  lv_label_set_text(weather.footer, wxExtra ?
                    "TAP TO VIEW DETAILS" : "WEATHER SYNCED FROM PHONE");
}
}
