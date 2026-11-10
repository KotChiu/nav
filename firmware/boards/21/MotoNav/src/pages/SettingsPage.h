#pragma once

void drawSettings() {
  gfx->fillScreen(C_UI_BG);
  onWaitScreen = false;
  frameIsMap = false;
  needFlush = true;
}
