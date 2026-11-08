#pragma once

// The robot is composed from small LVGL objects in LvglOverlay.h. Keep the
// canvas opaque so page changes never leave pixels from the previous screen.
void drawRobotPage() {
  gfx->fillScreen(C_BLACK);
  onWaitScreen = false;
  frameIsMap = false;
  needFlush = true;
}
