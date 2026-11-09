#pragma once
// 首页骑手动画和等待画面。由 MotoNav.ino 按依赖顺序包含，仅编译一次。
// ================= 整屏画面 =================

#define WAIT_READY  0
#define WAIT_NOLINK 1
#define WAIT_LOST   2

// Original pixel rider retired; the home instrument is fully LVGL-rendered.
bool onWaitScreen = false;
bool sliding = false;                       // v5.5 正在拖动/滑动换页：别的画面先别画
int waitSt = 0;
int riderPrevF = -1, riderPrevZ = -2;
bool catForce = false;                      // 首页刚重画：马上画一次

// 每 0.1 秒检查一次，样子变了才重画（函数名沿用 v3.6）
void animateCat() {
  if (!onWaitScreen || sliding) return;
  // Home is intentionally static on the RGB panel.
}

// Horizontal: weather / home / music. Vertical: settings / watch / robot / stocks.
int page = 0;                               // 0 home, 1 weather, 2 music, 3 settings, 4 watch, 5 robot, 6 stocks
unsigned long pageAt = 0;

// 页面顺序对应滑动位置：音乐在左，主页居中，天气在右；导航主页不显示。
void drawPageDots() {
  if (lvglUiReady) return;
  if (page == 0 && inNav) return;
  if (page >= 3 && page <= 5) {
    for (int i = 0; i < 3; i++) {
      aaDisc(CX - 20 + i * 20, 444, 4, page == 3 + i ? C_UI_TEXT : C_UI_DIM);
    }
    return;
  }
  const int pageOrder[] = {1, 0, 2};
  for (int i = 0; i < 3; i++) {
    aaDisc(CX + (i - 1) * 20, 444, 4, page == pageOrder[i] ? C_UI_TEXT : C_UI_DIM);
  }
}

// Low-contrast dial edge and a separate weather surface on the home screen.
void drawThemeRing() {
  for (int quadrant = 0; quadrant < 4; quadrant++) {
    const int start = 15 + quadrant * 90;
    gfx->fillArc(CX, CX, 210, 209, start, start + 60, C_UI_LINE);
  }
}

void drawWaiting(int st) {
  waitSt = st;
  if (sliding) return;                                                 // v5.5 正在滑动：滑完再画
  if (page == 2) { onWaitScreen = false; musicDirty = true; return; }
  if (page == 1) { onWaitScreen = false; wxDirty = true; return; }   // v5.3 正在看天气：先不画首页，滑回来时再画
  if (page >= 3 && page <= 6) { onWaitScreen = false; needFlush = true; return; }
  gfx->fillScreen(C_UI_BG);
  // Native LVGL orbit, clock and weather card own the idle screen. During
  // early boot the old dial ring is a harmless low-contrast fallback.
  if (!lvglUiReady) drawThemeRing();
  onWaitScreen = true;
  if (st == WAIT_LOST) drawRedRing();
  if (!lvglUiReady) {
    printCentered("MotoNav", CX, 200, 6, C_UI_TEXT);
    printCentered(st == WAIT_READY ? "ready" : st == WAIT_LOST ? "LINK LOST" : "no link",
                  CX, 275, 3, st == WAIT_READY ? C_UI_TEXT : st == WAIT_LOST ? C_UI_ACCENT2 : C_UI_SUB);
    printCentered(bleName, CX, 332, 2, C_UI_SUB);
    printCentered(String("fw ") + FW_VERSION, CX, 360, 2, C_UI_SUB);
    printCentered(String("rst ") + String((int)esp_reset_reason()), CX, 388, 1, C_UI_DIM);
    drawStatus();
    drawPageDots();
  }
  needFlush = true;
}
