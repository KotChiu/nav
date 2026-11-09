#pragma once
// Interactive Settings is its own UI controller, not a decorative brightness
// card. Display / Ride / Link / About use persisted device state, not mocks.
// Included from LvglOverlay.h after the firmware state declarations.
#include "VisualLanguage.h"

namespace MotoNavUI {
struct SettingsView {
  lv_obj_t *root = nullptr;
  lv_obj_t *tabs[4] = {};
  lv_obj_t *tabText[4] = {};
  lv_obj_t *sections[4] = {};
  lv_obj_t *brightness = nullptr;
  lv_obj_t *brightnessValue = nullptr;
  lv_obj_t *rotationValue = nullptr;
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
static void settingsRefresh() {
  if (!settings.root) return;
  char value[96];
  snprintf(value, sizeof(value), "%d%%", (screenBrightness * 100 + 90) / 180);
  lv_label_set_text(settings.brightnessValue, value);
  snprintf(value, sizeof(value), "%d DEG", screenDeg);
  lv_label_set_text(settings.rotationValue, value);
  lv_label_set_text(settings.cleanValue, cleanOn ? "ON" : "OFF");
  lv_label_set_text(settings.progressValue, progOn ? "ON" : "OFF");
  lv_label_set_text(settings.musicModeValue, hidMusicMode ? "HID" : "APP");
  lv_label_set_text(settings.deviceStatus,
                    bleConnected && bleAuthed ? "SECURELY CONNECTED" : "WAITING FOR PHONE");
  lv_label_set_text(settings.deviceName, bleName.c_str());
  lv_label_set_text(settings.firmware, FW_VERSION);
  snprintf(value, sizeof(value), "%u KB FREE",
           (unsigned)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));
  lv_label_set_text(settings.memory, value);
}
static void settingsSelectTab(int index) {
  if (!settings.root || index < 0 || index >= 4) return;
  settings.selected = index;
  for (int i = 0; i < 4; ++i) {
    bool active = i == index;
    lv_obj_set_style_bg_color(settings.tabs[i],
         color(active ? Mint : SurfaceRaised), 0);
    lv_obj_set_style_border_color(settings.tabs[i],
         color(active ? Mint : Line), 0);
    lv_obj_set_style_text_color(settings.tabText[i],
         color(active ? Background : Muted), 0);
    if (active) lv_obj_clear_flag(settings.sections[i], LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(settings.sections[i], LV_OBJ_FLAG_HIDDEN);
  }
  settingsRefresh();
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
    panel->setBrightness(screenBrightness);
    settingsRefresh();
  } else if (eventCode == LV_EVENT_RELEASED ||
             eventCode == LV_EVENT_PRESS_LOST) {
    lvglCaptureTouch = false;
    brightnessSaveAt = millis() + 500; // existing deferred persistence
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
  settingsText(parent, title, 23, y, &lv_font_montserrat_20, Text);
  settingsText(parent, sub, 23, y + 30, &lv_font_montserrat_16, Muted);
  lv_obj_t *b = button(parent, 231, y - 2, 88, 50, " ");
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
                             &lv_font_montserrat_16, Mint);
  lv_obj_align(eyebrow, LV_ALIGN_TOP_MID, 0, 42);
  lv_obj_set_style_text_letter_space(eyebrow, 2, 0);
  lv_obj_t *title = label(settings.root, "Settings",
                           &lv_font_montserrat_28, Text);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 71);

  static const char *names[4] = {"DISPLAY", "RIDE", "LINK", "ABOUT"};
  for (int i = 0; i < 4; ++i) {
    settings.tabs[i] = button(settings.root, 55 + i * 91, 124, 83, 43, "");
    settings.tabText[i] = lv_obj_get_child(settings.tabs[i], 0);
    lv_label_set_text(settings.tabText[i], names[i]);
    lv_obj_set_style_text_font(settings.tabText[i], &lv_font_montserrat_16, 0);
    lv_obj_add_event_cb(settings.tabs[i], settingsTabEvent,
                        LV_EVENT_CLICKED, (void *)(intptr_t)i);
    settings.sections[i] = panel(settings.root, 53, 178, 360, 199, 24);
  }

  lv_obj_t *display = settings.sections[0];
  settingsText(display, "BRIGHTNESS", 23, 17, &lv_font_montserrat_16, Muted);
  settings.brightnessValue = label(display, "70%", &lv_font_montserrat_20, Mint);
  lv_obj_align(settings.brightnessValue, LV_ALIGN_TOP_RIGHT, -23, 14);
  settings.brightness = lv_slider_create(display);
  lv_obj_set_size(settings.brightness, 306, 18);
  lv_obj_set_pos(settings.brightness, 27, 56);
  lv_slider_set_range(settings.brightness, 5, 180);
  lv_slider_set_value(settings.brightness, screenBrightness, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(settings.brightness, color(Line), LV_PART_MAIN);
  lv_obj_set_style_bg_color(settings.brightness, color(Mint), LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(settings.brightness, color(Text), LV_PART_KNOB);
  lv_obj_set_style_pad_all(settings.brightness, 7, LV_PART_KNOB);
  lv_obj_set_style_border_width(settings.brightness, 0, LV_PART_MAIN);
  lv_obj_add_event_cb(settings.brightness, settingsBrightnessEvent,
                      LV_EVENT_ALL, nullptr);
  settingsAction(display, 106, "Orientation", "Tap to rotate clockwise",
                 &settings.rotationValue, 0);

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
  settings.memory = label(about, "-- KB", &lv_font_montserrat_16, Mint);
  lv_obj_set_pos(settings.memory, 163, 148);

  lv_obj_t *foot = label(settings.root, "SWIPE DOWN  /  HOME",
                          &lv_font_montserrat_16, Faint);
  lv_obj_align(foot, LV_ALIGN_TOP_MID, 0, 397);
  settingsSelectTab(0);
}
static void settingsUpdate(bool visible) {
  if (!settings.root) return;
  if (visible) lv_obj_clear_flag(settings.root, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_add_flag(settings.root, LV_OBJ_FLAG_HIDDEN);
  if (visible) {
    if (!lv_obj_has_state(settings.brightness, LV_STATE_PRESSED) &&
        lv_slider_get_value(settings.brightness) != screenBrightness)
      lv_slider_set_value(settings.brightness, screenBrightness, LV_ANIM_OFF);
    static uint32_t lastUpdated = 0;
    if (!lastUpdated || millis() - lastUpdated > 950) {
      lastUpdated = millis();
      settingsRefresh();
    }
  }
}
} // namespace MotoNavUI
