#pragma once
// Interactive Settings is its own UI controller, not a decorative brightness
// card. Display / Ride / Link / About use persisted device state, not mocks.
// Included from LvglOverlay.h after the firmware state declarations.
#include "VisualLanguage.h"
#include "LvglIcons.h"
#include <cstring>

namespace MotoNavUI {
struct SettingsView {
  lv_obj_t *root = nullptr;
  lv_obj_t *tabs[4] = {};
  lv_obj_t *tabText[4] = {};
  lv_obj_t *sections[4] = {};
  lv_obj_t *brightness = nullptr;
  lv_obj_t *brightnessValue = nullptr;
  lv_obj_t *rotationValue = nullptr;
  lv_obj_t *rotation = nullptr;
  lv_obj_t *cleanValue = nullptr;
  lv_obj_t *progressValue = nullptr;
  lv_obj_t *musicModeValue = nullptr;
  lv_obj_t *deviceStatus = nullptr;
  lv_obj_t *deviceName = nullptr;
  lv_obj_t *firmware = nullptr;
  lv_obj_t *memory = nullptr;
  int selected = 0;
};
static SettingsView settings;

static void settingsText(lv_obj_t *parent, const char *str, int x, int y,
                         const lv_font_t *font, uint32_t tint = Text) {
  lv_obj_t *l = label(parent, str, font, tint);
  lv_obj_set_pos(l, x, y);
}
static void settingsSetText(lv_obj_t *label, const char *text) {
  if (label && text && std::strcmp(lv_label_get_text(label), text) != 0)
    lv_label_set_text(label, text);
}
static void settingsToggle(lv_obj_t *value,bool enabled) {
  if(!value) return;
  lv_obj_t *control=lv_obj_get_parent(value);
  if(enabled) lv_obj_add_state(control,LV_STATE_CHECKED);
  else lv_obj_clear_state(control,LV_STATE_CHECKED);
  lv_obj_set_style_bg_color(control,color(Accent),LV_STATE_CHECKED);
  lv_obj_set_style_bg_color(control,color(Accent),LV_STATE_PRESSED);
  lv_obj_set_style_border_color(control,color(Accent),LV_STATE_CHECKED);
  lv_obj_set_style_text_color(value,color(enabled?Background:Text),0);
  lv_obj_center(value);
}
static void settingsRefresh() {
  if (!settings.root) return;
  char value[96];
  snprintf(value, sizeof(value), "%d%%", (screenBrightness * 100 + 90) / 180);
  settingsSetText(settings.brightnessValue, value);
  snprintf(value, sizeof(value), "%d DEG", screenDeg);
  if (!lv_obj_has_state(settings.rotation, LV_STATE_PRESSED))
    settingsSetText(settings.rotationValue, value);
  settingsSetText(settings.cleanValue, cleanOn ? "ON" : "OFF");
  settingsSetText(settings.progressValue, progOn ? "ON" : "OFF");
  settingsSetText(settings.musicModeValue, hidMusicMode ? "HID" : "APP");
  settingsToggle(settings.cleanValue,cleanOn);
  settingsToggle(settings.progressValue,progOn);
  settingsToggle(settings.musicModeValue,hidMusicMode);
  settingsSetText(settings.deviceStatus,
                    bleConnected && bleAuthed ? "SECURELY CONNECTED" : "WAITING FOR PHONE");
  lv_obj_set_style_text_color(settings.deviceStatus,
                  color(bleConnected && bleAuthed ? Cool : Warm), 0);
  settingsSetText(settings.deviceName, bleName.c_str());
  settingsSetText(settings.firmware, FW_VERSION);
  snprintf(value, sizeof(value), "%u KB FREE",
           (unsigned)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));
  settingsSetText(settings.memory, value);
}
static void settingsSelectTab(int index, bool scroll=true) {
  if (!settings.root || index < 0 || index >= 4) return;
  settings.selected = index;
  for (int i = 0; i < 4; ++i) {
    bool active = i == index;
    lv_obj_set_style_bg_color(settings.tabs[i],
         color(active ? Accent : SurfaceRaised), 0);
    lv_obj_set_style_border_color(settings.tabs[i],
         color(active ? Accent : Line), 0);
    lv_obj_set_style_text_color(settings.tabText[i],
         color(active ? Background : Muted), 0);
    lv_obj_clear_flag(settings.sections[i], LV_OBJ_FLAG_HIDDEN);
  }
  if(scroll) lv_obj_scroll_to_y(lv_obj_get_parent(settings.sections[index]), index * 218, LV_ANIM_OFF);
  settingsRefresh();
}
static void settingsScrollEvent(lv_event_t *event) {
  const int y=lv_obj_get_scroll_y(lv_event_get_target(event));
  const int selected=constrain((y+109)/218,0,3);
  if(selected!=settings.selected) settingsSelectTab(selected,false);
}
static void settingsTabEvent(lv_event_t *event) {
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
  int index = (int)(intptr_t)lv_event_get_user_data(event);
  settingsSelectTab(index);
}
static void settingsBrightnessEvent(lv_event_t *event) {
  const lv_event_code_t eventCode = lv_event_get_code(event);
  if (eventCode == LV_EVENT_PRESSED) lvglCaptureTouch = true;
  else if (eventCode == LV_EVENT_VALUE_CHANGED) {
    screenBrightness = constrain((int)lv_slider_get_value(settings.brightness), 5, 180);
    ::panel->setBrightness(screenBrightness);
    settingsRefresh();
  } else if (eventCode == LV_EVENT_RELEASED ||
             eventCode == LV_EVENT_PRESS_LOST) {
    lvglCaptureTouch = false;
    brightnessSaveAt = millis() + 500; // existing deferred persistence
  }
}
static void settingsRotationEvent(lv_event_t *event) {
  const auto code = lv_event_get_code(event);
  if (code == LV_EVENT_PRESSED) lvglCaptureTouch = true;
  if (code == LV_EVENT_VALUE_CHANGED) {
    char value[32];
    snprintf(value, sizeof(value), "%d DEG", (int)lv_slider_get_value(settings.rotation));
    settingsSetText(settings.rotationValue, value);
  }
  if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    // Apply on release: changing the coordinate transform under a held finger
    // would move the slider away from the gesture that controls it.
    setScreenDeg(lv_slider_get_value(settings.rotation), true);
    lvglCaptureTouch = false;
    forceFullFlush = true; needFlush = true;
    settingsRefresh();
  }
}
static void settingsActionEvent(lv_event_t *event) {
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
  const int action = (int)(intptr_t)lv_event_get_user_data(event);
  switch (action) {
    case 0: // saved rotation; the display pipeline already rotates touches.
      setScreenDeg((screenDeg + 90) % 360, true);
      forceFullFlush = true; needFlush = true;
      break;
    case 1:
      cleanOn = !cleanOn;
      navDirty = true;
      { Preferences p; if (p.begin("motonav", false)) {
        p.putBool("cleanmap", cleanOn); p.end();
      }}
      break;
    case 2:
      progOn = !progOn;
      navDirty = true;
      { Preferences p; if (p.begin("motonav", false)) {
        p.putBool("progring", progOn); p.end();
      }}
      break;
    case 3: // Matches the existing #HM protocol side effects.
      hidMusicMode = !hidMusicMode;
      hidPulses.cancelQueued();
      if (!hidMusicMode) {
        musicLyrics = false;
        imgs[12].showW = 0; imgs[12].showH = 0; imgs[12].recvId = -1;
      }
      musicDirty = true;
      pendingHidStatus = String("HID,") + (hidMusicMode ? 1 : 0) +
                         "," + (hidMediaReady() ? 1 : 0);
      { Preferences p; if (p.begin("motonav", false)) {
        p.putBool("mediahid", hidMusicMode); p.end();
      }}
      break;
    default: return;
  }
  settingsRefresh();
}
static lv_obj_t *settingsAction(lv_obj_t *parent, int y, const char *title,
                                 const char *sub, lv_obj_t **state,
                                 int code) {
  const Icon glyph = code == 0 ? Icon::Screen :
                     code == 1 ? Icon::Map :
                     code == 2 ? Icon::Ride : Icon::Music;
  icon(parent, glyph, 18, y + 10, Accent);
  lv_obj_t *heading = label(parent, title, &lv_font_montserrat_20, Text);
  lv_obj_set_pos(heading, 57, y);
  lv_obj_set_size(heading, 156, 29);
  lv_label_set_long_mode(heading, LV_LABEL_LONG_DOT);
  lv_obj_t *detail = label(parent, sub, &lv_font_montserrat_16, Muted);
  lv_obj_set_pos(detail, 57, y + 30);
  lv_obj_set_size(detail, 156, 23);
  lv_label_set_long_mode(detail, LV_LABEL_LONG_DOT);
  lv_obj_t *b = button(parent, 227, y - 2, 70, 50, " ");
  lv_obj_t *value = lv_obj_get_child(b, 0);
  lv_obj_set_style_text_font(value, &lv_font_montserrat_16, 0);
  lv_obj_add_event_cb(b, settingsActionEvent, LV_EVENT_CLICKED, (void *)(intptr_t)code);
  *state = value;
  return b;
}
static void settingsCreate(lv_obj_t *screen) {
  settings.root = lv_obj_create(screen);
  lv_obj_remove_style_all(settings.root);
  lv_obj_set_size(settings.root, Width, Width);
  noInteraction(settings.root);
  lv_obj_add_flag(settings.root, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t *eyebrow = label(settings.root, "MOTONAV / SYSTEM",
                             &lv_font_montserrat_16, Accent);
  lv_obj_align(eyebrow, LV_ALIGN_TOP_MID, 0, 42);
  lv_obj_set_style_text_letter_space(eyebrow, 2, 0);
  lv_obj_t *title = label(settings.root, "Settings",
                           &lv_font_montserrat_28, Text);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 71);

  static const char *names[4] = {"DISPLAY", "RIDE", "LINK", "ABOUT"};
  for (int i = 0; i < 4; ++i) {
    settings.tabs[i] = button(settings.root, 55 + i * 91, 124, 83, 47, "");
    settings.tabText[i] = lv_obj_get_child(settings.tabs[i], 0);
    settingsSetText(settings.tabText[i], names[i]);
    lv_obj_set_style_text_font(settings.tabText[i], &lv_font_montserrat_16, 0);
    lv_obj_center(settings.tabText[i]);
    lv_obj_add_event_cb(settings.tabs[i], settingsTabEvent,
                        LV_EVENT_CLICKED, (void *)(intptr_t)i);
  }
  // Real scrollable cards; every section has content and reachable controls.
  lv_obj_t *list = lv_obj_create(settings.root);
  lv_obj_remove_style_all(list);
  lv_obj_set_style_bg_color(list, color(Background), 0);
  lv_obj_set_style_bg_opa(list, LV_OPA_COVER, 0);
  lv_obj_set_pos(list, 73, 176);
  lv_obj_set_size(list, 320, 224);
  lv_obj_set_scroll_dir(list, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);
  lv_obj_add_event_cb(list,settingsScrollEvent,LV_EVENT_SCROLL,nullptr);
  for (int i = 0; i < 4; ++i)
    settings.sections[i] = panel(list, 0, i * 218, 320, 202, 24);

  lv_obj_t *display = settings.sections[0];
  settingsText(display, "BRIGHTNESS", 23, 17, &lv_font_montserrat_16, Muted);
  settings.brightnessValue = label(display, "70%", &lv_font_montserrat_20, Accent);
  lv_obj_align(settings.brightnessValue, LV_ALIGN_TOP_RIGHT, -23, 14);
  settings.brightness = lv_slider_create(display);
  lv_obj_set_size(settings.brightness, 266, 18);
  lv_obj_set_pos(settings.brightness, 27, 56);
  lv_slider_set_range(settings.brightness, 5, 180);
  lv_slider_set_value(settings.brightness, screenBrightness, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(settings.brightness, color(Line), LV_PART_MAIN);
  lv_obj_set_style_bg_color(settings.brightness, color(Accent), LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(settings.brightness, color(Text), LV_PART_KNOB);
  lv_obj_set_style_pad_all(settings.brightness, 7, LV_PART_KNOB);
  lv_obj_set_style_border_width(settings.brightness, 0, LV_PART_MAIN);
  lv_obj_add_event_cb(settings.brightness, settingsBrightnessEvent,
                      LV_EVENT_ALL, nullptr);
  settingsText(display, "ROTATION", 23, 100, &lv_font_montserrat_16, Muted);
  settings.rotationValue = label(display, "0 DEG", &lv_font_montserrat_16, Accent);
  lv_obj_align(settings.rotationValue, LV_ALIGN_TOP_RIGHT, -23, 100);
  settings.rotation = lv_slider_create(display);
  lv_obj_set_pos(settings.rotation, 27, 151);
  lv_obj_set_size(settings.rotation, 266, 18);
  lv_obj_set_style_bg_color(settings.rotation, color(Line), LV_PART_MAIN);
  lv_obj_set_style_bg_color(settings.rotation, color(Accent), LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(settings.rotation, color(Text), LV_PART_KNOB);
  lv_slider_set_range(settings.rotation, 0, 359);
  lv_slider_set_value(settings.rotation, screenDeg, LV_ANIM_OFF);
  lv_obj_add_event_cb(settings.rotation, settingsRotationEvent, LV_EVENT_ALL, nullptr);

  lv_obj_t *ride = settings.sections[1];
  settingsAction(ride, 22, "Minimal map", "Hide secondary road alerts",
                 &settings.cleanValue, 1);
  settingsAction(ride, 112, "Trip ring", "Show journey progress",
                 &settings.progressValue, 2);

  lv_obj_t *link = settings.sections[2];
  settingsText(link, "PHONE CONNECTION", 23, 20,
               &lv_font_montserrat_16, Muted);
  settings.deviceStatus = label(link, "WAITING FOR PHONE",
                                &lv_font_montserrat_20, Text);
  lv_obj_set_pos(settings.deviceStatus, 23, 49);
  lv_obj_set_width(settings.deviceStatus, 274);
  lv_label_set_long_mode(settings.deviceStatus, LV_LABEL_LONG_DOT);
  settingsAction(link, 111, "Music keys", "HID / App control",
                 &settings.musicModeValue, 3);

  lv_obj_t *about = settings.sections[3];
  settingsText(about, "DEVICE NAME", 22, 17,
               &lv_font_montserrat_16, Muted);
  settings.deviceName = label(about, "MotoNav", &lv_font_montserrat_20, Text);
  lv_obj_set_pos(settings.deviceName, 22, 44);
  settingsText(about, "FIRMWARE", 22, 83, &lv_font_montserrat_16, Muted);
  settings.firmware = label(about, FW_VERSION, &lv_font_montserrat_20, Text);
  lv_obj_set_pos(settings.firmware, 22, 109);
  settingsText(about, "FREE PSRAM", 22, 148, &lv_font_montserrat_16, Muted);
  settings.memory = label(about, "-- KB", &lv_font_montserrat_16, Accent);
  lv_obj_set_pos(settings.memory, 163, 148);

  lv_obj_t *foot = label(settings.root, "",
                          &lv_font_montserrat_16, Faint);
  lv_obj_align(foot, LV_ALIGN_TOP_MID, 0, 397);
  settingsSelectTab(0);
}
static void settingsUpdate(bool visible) {
  if (!settings.root) return;
  const bool hidden = lv_obj_has_flag(settings.root, LV_OBJ_FLAG_HIDDEN);
  if (visible && hidden) lv_obj_clear_flag(settings.root, LV_OBJ_FLAG_HIDDEN);
  else if (!visible && !hidden) lv_obj_add_flag(settings.root, LV_OBJ_FLAG_HIDDEN);
  if (visible) {
    if (!lv_obj_has_state(settings.brightness, LV_STATE_PRESSED) &&
        lv_slider_get_value(settings.brightness) != screenBrightness)
      lv_slider_set_value(settings.brightness, screenBrightness, LV_ANIM_OFF);
    if (!lv_obj_has_state(settings.rotation, LV_STATE_PRESSED) &&
        lv_slider_get_value(settings.rotation) != screenDeg)
      lv_slider_set_value(settings.rotation, screenDeg, LV_ANIM_OFF);
    static uint32_t lastUpdated = 0;
    if (!lastUpdated || millis() - lastUpdated > 950) {
      lastUpdated = millis();
      settingsRefresh();
    }
  }
}
} // namespace MotoNavUI
