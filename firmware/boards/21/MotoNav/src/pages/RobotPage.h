#pragma once

// The robot is composed from small LVGL objects in LvglOverlay.h. Keep the
// canvas opaque so page changes never leave pixels from the previous screen.
// Tap cycles six eye expressions; hold sleeps/wakes. Idle only changes eyes.
void drawRobotPage() {
  gfx->fillScreen(C_UI_BG);
  onWaitScreen = false;
  frameIsMap = false;
  needFlush = true;
}
