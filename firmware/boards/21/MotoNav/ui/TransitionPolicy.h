#pragma once
// Progressive, testable rollout of LVGL snapshot transitions.
// All completed page snapshots are composited as native LVGL images, but
// navigation-active slides remain on the proven legacy path until road QA.
#ifndef MOTONAV_NATIVE_NAV_SLIDES
#define MOTONAV_NATIVE_NAV_SLIDES 0
#endif
static inline bool motonavNativeSwipeAllowed(
    int sourcePage, int targetPage, bool navigating, bool initialized) {
  if (!initialized || sourcePage == targetPage ||
      sourcePage < 0 || sourcePage > 5 ||
      targetPage < 0 || targetPage > 5) return false;
  if (navigating && !MOTONAV_NATIVE_NAV_SLIDES) return false;
  return true;
}
