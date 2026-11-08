#pragma once

void drawSettings() {
  gfx->fillScreen(C_BLACK);
  onWaitScreen = false;
  frameIsMap = false;
  needFlush = true;
}
