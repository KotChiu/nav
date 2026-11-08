#pragma once

// Quotes are fetched by the iPhone and rendered by small LVGL labels.
void drawStockPage() {
  gfx->fillScreen(C_BLACK);
  stockPaintedRevision = stockRevision;
  onWaitScreen = false;
  frameIsMap = false;
  needFlush = true;
}
