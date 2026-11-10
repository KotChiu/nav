#pragma once
// Navigation is a LIVE MAP, never a speed-dial mock. These two small chips
// only identify the active map feed / verified phone link, and intentionally
// leave the route, position, road labels, lanes and maneuver HUD unobstructed.
#include "VisualLanguage.h"
#include "LvglIcons.h"

namespace MotoNavUI {
struct MapOverlayView {
  lv_obj_t *root = nullptr;
  lv_obj_t *mapChip = nullptr;
  lv_obj_t *mapLabel = nullptr;
  lv_obj_t *linkChip = nullptr;
  lv_obj_t *linkLabel = nullptr;
  bool wasVisible = false;
  bool hadMap = false;
  bool wasConnected = false;
};
static MapOverlayView mapOverlay;

static void mapOverlayCreate(lv_obj_t *screen) {
  mapOverlay.root = lv_obj_create(screen);
  lv_obj_remove_style_all(mapOverlay.root);
  lv_obj_set_size(mapOverlay.root, Width, Width);
  noInteraction(mapOverlay.root);

  // Both chips are inside the circular safe boundary even at y=100.
  mapOverlay.mapChip = panel(mapOverlay.root, 58, 99, 109, 35, 17,
                             Background);
  icon(mapOverlay.mapChip, Icon::Map, 8, 5, Accent);
  mapOverlay.mapLabel = label(mapOverlay.mapChip, "MAP",
                              &lv_font_montserrat_16, Text);
  lv_obj_set_pos(mapOverlay.mapLabel, 37, 9);

  mapOverlay.linkChip = panel(mapOverlay.root, 304, 99, 104, 35, 17,
                              Background);
  icon(mapOverlay.linkChip, Icon::Link, 8, 5, Cool);
  mapOverlay.linkLabel = label(mapOverlay.linkChip, "LINK",
                               &lv_font_montserrat_16, Text);
  lv_obj_set_pos(mapOverlay.linkLabel, 37, 9);
  lv_obj_add_flag(mapOverlay.root, LV_OBJ_FLAG_HIDDEN);
}

static void mapOverlayUpdate(bool visible, bool actualMapData,
                             bool authenticatedPhone) {
  if (!mapOverlay.root) return;
  if (mapOverlay.wasVisible != visible) {
    mapOverlay.wasVisible = visible;
    if (visible) {
      lv_obj_clear_flag(mapOverlay.root, LV_OBJ_FLAG_HIDDEN);
      lv_obj_move_foreground(mapOverlay.root);
    } else {
      lv_obj_add_flag(mapOverlay.root, LV_OBJ_FLAG_HIDDEN);
    }
  }
  if (!visible) return;
  if (mapOverlay.hadMap != actualMapData) {
    mapOverlay.hadMap = actualMapData;
    lv_label_set_text(mapOverlay.mapLabel, actualMapData ? "MAP" : "SYNC");
    lv_obj_set_style_text_color(mapOverlay.mapLabel,
                                color(actualMapData ? Text : Warm), 0);
  }
  if (mapOverlay.wasConnected != authenticatedPhone) {
    mapOverlay.wasConnected = authenticatedPhone;
    lv_label_set_text(mapOverlay.linkLabel, authenticatedPhone ? "LINK" : "OFF");
    lv_obj_set_style_text_color(mapOverlay.linkLabel,
                                color(authenticatedPhone ? Cool : Warm), 0);
  }
}
} // namespace MotoNavUI
