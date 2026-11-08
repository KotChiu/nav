#pragma once

void drawWatchFace() {
  gfx->fillScreen(C_BLACK);
  onWaitScreen = false;
  frameIsMap = false;
  needFlush = true;
}
