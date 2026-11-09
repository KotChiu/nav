// MotoNav 2.1 LVGL desktop renderer.
// It uses the exact native LVGL HomeChrome used in the device firmware.
// Other screens use explicitly simulated fixtures; no ESP32, BLE or road claims.
#include <SDL.h>
#include <lvgl.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>

#include "../../firmware/boards/21/MotoNav/ui/LvglHomeChrome.h"

namespace {
constexpr int W = 466, H = 466, PANEL = 480, BORDER = (PANEL - W) / 2;
constexpr uint16_t CYAN = 0x04B6, MAGENTA = 0xF815;
constexpr uint32_t BG = 0x000000, PANEL_BG = 0x101820, SUB = 0x98A5B4;
lv_color_t renderBuffer[W * 36];
uint16_t framebuffer[W * H] = {};
uint16_t roundedFrame[W * H] = {};
lv_disp_draw_buf_t lvDraw;
lv_disp_drv_t lvDisp;
lv_indev_drv_t lvPointer;
int cursorX = W / 2, cursorY = H / 2;
bool pressed = false, dirty = true;
int pageIndex = 0, brightness = 70, speed = 68;
bool navigation = false, linkReady = true, playing = true;
bool dragging = false;
int downX = 0, downY = 0;
MotoNavHomeChrome chrome;
lv_obj_t *brightnessValue = nullptr;
lv_obj_t *musicState = nullptr;
static const char *pageNames[] = {"HOME", "WEATHER", "MUSIC", "COMPANION", "STOCKS", "SETTINGS"};

lv_obj_t *label(lv_obj_t *parent, const char *text, int x, int y,
                uint32_t color = 0xFFFFFF, const lv_font_t *font = &lv_font_montserrat_20) {
  lv_obj_t *l = lv_label_create(parent);
  lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
  lv_obj_set_style_text_font(l, font, 0);
  lv_label_set_text(l, text);
  lv_obj_set_pos(l, x, y);
  return l;
}
void centerLabel(lv_obj_t *parent, const char *text, int y,
                 uint32_t color = 0xFFFFFF, const lv_font_t *font = &lv_font_montserrat_20) {
  lv_obj_t *l = label(parent, text, 0, y, color, font);
  lv_obj_align(l, LV_ALIGN_TOP_MID, 0, y);
}
lv_obj_t *box(lv_obj_t *parent, int x, int y, int w, int h, uint32_t bg,
              int radius = 10, uint32_t stroke = 0) {
  lv_obj_t *b = lv_obj_create(parent);
  lv_obj_remove_style_all(b);
  lv_obj_set_pos(b, x, y);
  lv_obj_set_size(b, w, h);
  lv_obj_set_style_radius(b, radius, 0);
  lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(b, lv_color_hex(bg), 0);
  if (stroke) {
    lv_obj_set_style_border_color(b, lv_color_hex(stroke), 0);
    lv_obj_set_style_border_width(b, 1, 0);
  }
  lv_obj_clear_flag(b, LV_OBJ_FLAG_SCROLLABLE);
  return b;
}
void showPage(int target);
void pageEvent(lv_event_t *ev) {
  if (lv_event_get_code(ev) != LV_EVENT_CLICKED) return;
  const int destination = static_cast<int>(reinterpret_cast<intptr_t>(lv_event_get_user_data(ev)));
  showPage(destination);
}
void navEvent(lv_event_t *ev) {
  if (lv_event_get_code(ev) != LV_EVENT_CLICKED) return;
  navigation = !navigation;
  showPage(0);
}
lv_obj_t *button(lv_obj_t *parent, const char *text, int x, int y, int w,
                 lv_event_cb_t action, int argument = 0) {
  lv_obj_t *b = lv_btn_create(parent);
  lv_obj_set_size(b, w, 44);
  lv_obj_set_pos(b, x, y);
  lv_obj_set_style_radius(b, 11, 0);
  lv_obj_set_style_bg_color(b, lv_color_hex(PANEL_BG), 0);
  lv_obj_set_style_border_width(b, 1, 0);
  lv_obj_set_style_border_color(b, lv_color_hex(0x0CCADB), 0);
  lv_obj_set_style_shadow_width(b, 0, 0);
  lv_obj_add_event_cb(b, action, LV_EVENT_CLICKED, reinterpret_cast<void *>(static_cast<intptr_t>(argument)));
  lv_obj_t *textObj = lv_label_create(b);
  lv_label_set_text(textObj, text);
  lv_obj_set_style_text_color(textObj, lv_color_white(), 0);
  lv_obj_set_style_text_font(textObj, &lv_font_montserrat_14, 0);
  lv_obj_center(textObj);
  return b;
}
void footer(lv_obj_t *screen) {
  label(screen, "MOCK DATA", 185, 421, 0x697987, &lv_font_montserrat_14);
  for (int i = 0; i < 6; ++i) {
    lv_obj_t *dot = box(screen, 173 + 23 * i, 445, 7, 7,
                        i == pageIndex ? 0xFFFFFF : 0x30414D, 7);
    (void)dot;
  }
}
void home(lv_obj_t *screen) {
  motonavHomeChromeCreate(chrome, screen, W / 2, CYAN, MAGENTA);
  motonavHomeChromeVisible(chrome, true);
  centerLabel(screen, "MOTO / NAV", 51, 0xFFFFFF, &lv_font_montserrat_20);
  std::time_t now = std::time(nullptr);
  std::tm local = *std::localtime(&now);
  char clock[12];
  std::strftime(clock, sizeof(clock), "%H:%M", &local);
  centerLabel(screen, clock, 108, 0xFFFFFF, &lv_font_montserrat_40);
  centerLabel(screen, "COMPANION DISPLAY", 166, SUB, &lv_font_montserrat_14);
  centerLabel(screen, linkReady ? "●  PHONE CONNECTED" : "○  PHONE OFFLINE",
              221, linkReady ? 0x26E3EE : 0xE75291, &lv_font_montserrat_16);
  centerLabel(screen, "24°   FAIR WEATHER", 266, 0xFFFFFF, &lv_font_montserrat_20);
  button(screen, "NAVIGATION", 92, 350, 133, navEvent);
  button(screen, "WEATHER", 240, 350, 133, pageEvent, 1);
}
void navigationPage(lv_obj_t *screen) {
  centerLabel(screen, "SIMULATED NAVIGATION", 53, 0x29DCE9, &lv_font_montserrat_16);
  char text[24];
  std::snprintf(text, sizeof(text), "%d", speed);
  centerLabel(screen, text, 93, 0xFFFFFF, &lv_font_montserrat_40);
  centerLabel(screen, "km/h", 147, SUB, &lv_font_montserrat_16);
  static lv_point_t route[] = {{96, 277}, {190, 277}, {190, 220}, {294, 220}, {294, 178}};
  lv_obj_t *line = lv_line_create(screen);
  lv_line_set_points(line, route, sizeof(route) / sizeof(route[0]));
  lv_obj_set_style_line_color(line, lv_color_hex(0x19DCEB), 0);
  lv_obj_set_style_line_width(line, 12, 0);
  lv_obj_set_style_line_rounded(line, true, 0);
  centerLabel(screen, "TURN RIGHT IN 250m", 304, 0xFFFFFF, &lv_font_montserrat_20);
  button(screen, "END MOCK ROUTE", 125, 363, 216, navEvent);
}
void weather(lv_obj_t *screen) {
  centerLabel(screen, "WEATHER", 50, 0x25DFED, &lv_font_montserrat_28);
  centerLabel(screen, "24°", 106, 0xFFFFFF, &lv_font_montserrat_40);
  centerLabel(screen, "SUNNY  ·  26% RAIN", 173, SUB, &lv_font_montserrat_16);
  box(screen, 55, 235, 356, 113, PANEL_BG, 14, 0x203B47);
  const int bars[9] = {13, 18, 14, 24, 35, 26, 18, 9, 6};
  for (int i = 0; i < 9; ++i) {
    box(screen, 83 + 35 * i, 321 - bars[i], 13, bars[i], 0x23D5E4, 4);
  }
  centerLabel(screen, "NEXT 60 MIN  ·  SIMULATED", 365, SUB, &lv_font_montserrat_14);
}
void musicEvent(lv_event_t *ev) {
  if (lv_event_get_code(ev) != LV_EVENT_CLICKED) return;
  playing = !playing;
  if (musicState) lv_label_set_text(musicState, playing ? "PLAYING" : "PAUSED");
}
void music(lv_obj_t *screen) {
  centerLabel(screen, "MUSIC", 50, 0x27DBE9, &lv_font_montserrat_28);
  lv_obj_t *disc = box(screen, 136, 111, 194, 194, 0x111F2A, LV_RADIUS_CIRCLE, 0x1ED7E8);
  (void)disc;
  centerLabel(screen, "♪", 161, 0xF225A8, &lv_font_montserrat_40);
  centerLabel(screen, "DEMO AUDIO", 310, 0xFFFFFF, &lv_font_montserrat_20);
  musicState = lv_label_create(screen);
  lv_label_set_text(musicState, playing ? "PLAYING" : "PAUSED");
  lv_obj_set_style_text_color(musicState, lv_color_hex(SUB), 0);
  lv_obj_align(musicState, LV_ALIGN_TOP_MID, 0, 340);
  button(screen, "PREV", 78, 374, 92, musicEvent);
  button(screen, "PLAY / PAUSE", 184, 374, 118, musicEvent);
  button(screen, "NEXT", 316, 374, 78, musicEvent);
}
void companion(lv_obj_t *screen) {
  centerLabel(screen, "COMPANION", 52, 0x26DEEB, &lv_font_montserrat_28);
  box(screen, 98, 108, 270, 270, 0x081119, LV_RADIUS_CIRCLE, 0x27D5EA);
  box(screen, 142, 167, 182, 148, 0x102630, 35);
  box(screen, 170, 219, 35, 45, 0x26DCEA, 12);
  box(screen, 260, 219, 35, 45, 0x26DCEA, 12);
  centerLabel(screen, "READY TO RIDE", 348, 0xF238A1, &lv_font_montserrat_16);
}
void stocks(lv_obj_t *screen) {
  centerLabel(screen, "MARKET  |  DAY", 47, 0x27DCEA, &lv_font_montserrat_24);
  const char *names[] = {"SH000001", "SZA", "AAPL"};
  const char *prices[] = {"3,182.56", "12,406.31", "278.40"};
  const char *changes[] = {"+0.78%", "-0.32%", "+1.26%"};
  for (int i = 0; i < 3; ++i) {
    int y = 107 + 105 * i;
    box(screen, 44, y, 378, 96, 0x0A131C, 12, 0x20313A);
    label(screen, names[i], 61, y+12, 0xFFFFFF, &lv_font_montserrat_16);
    label(screen, prices[i], 61, y+43, 0xFFFFFF, &lv_font_montserrat_24);
    label(screen, changes[i], 311, y+48, i==1?0x3DE0A1:0xFF627D, &lv_font_montserrat_16);
  }
}
void brightnessEvent(lv_event_t *ev) {
  if (lv_event_get_code(ev) != LV_EVENT_VALUE_CHANGED) return;
  brightness = lv_slider_get_value(lv_event_get_target(ev));
  if (brightnessValue) {
    char text[32];
    std::snprintf(text, sizeof(text), "%d%%", brightness);
    lv_label_set_text(brightnessValue, text);
  }
}
void settings(lv_obj_t *screen) {
  centerLabel(screen, "SETTINGS", 57, 0x2BDCE9, &lv_font_montserrat_28);
  box(screen, 63, 154, 340, 180, PANEL_BG, 14, 0x243A45);
  label(screen, "Display brightness", 93, 180, 0xFFFFFF, &lv_font_montserrat_20);
  lv_obj_t *slider = lv_slider_create(screen);
  lv_obj_set_size(slider, 270, 24);
  lv_obj_set_pos(slider, 97, 249);
  lv_slider_set_range(slider, 5, 100);
  lv_slider_set_value(slider, brightness, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(slider, lv_color_hex(0x13CBDD), LV_PART_INDICATOR);
  lv_obj_add_event_cb(slider, brightnessEvent, LV_EVENT_VALUE_CHANGED, nullptr);
  brightnessValue = lv_label_create(screen);
  lv_obj_set_style_text_color(brightnessValue, lv_color_white(), 0);
  lv_obj_align(brightnessValue, LV_ALIGN_TOP_MID, 0, 288);
  char value[20];
  std::snprintf(value, sizeof(value), "%d%%", brightness);
  lv_label_set_text(brightnessValue, value);
  centerLabel(screen, "PC SIMULATION - NO HARDWARE CONTROL", 362, SUB, &lv_font_montserrat_14);
}
void showPage(int target) {
  pageIndex = (target + 6) % 6;
  brightnessValue = nullptr; musicState = nullptr;
  lv_obj_t *screen = lv_scr_act();
  lv_obj_clean(screen);
  lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
  switch (pageIndex) {
    case 0: navigation ? navigationPage(screen) : home(screen); break;
    case 1: weather(screen); break;
    case 2: music(screen); break;
    case 3: companion(screen); break;
    case 4: stocks(screen); break;
    case 5: settings(screen); break;
  }
  if (pageIndex != 0 || !navigation) footer(screen);
  dirty = true;
}
void flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *colors) {
  const int width = area->x2 - area->x1 + 1;
  for (int y = area->y1; y <= area->y2; ++y) {
    if (y < 0 || y >= H || area->x1 < 0 || area->x2 >= W) continue;
    std::memcpy(framebuffer + y * W + area->x1, colors + (y - area->y1) * width,
                width * sizeof(lv_color_t));
  }
  dirty = true;
  lv_disp_flush_ready(drv);
}
void readPointer(lv_indev_drv_t *, lv_indev_data_t *data) {
  data->state = pressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
  data->point.x = cursorX;
  data->point.y = cursorY;
}
void render(SDL_Renderer *renderer, SDL_Texture *texture) {
  std::memcpy(roundedFrame, framebuffer, sizeof(roundedFrame));
  const int radius = W / 2, radiusSquared = (radius - 2) * (radius - 2);
  for (int y = 0; y < H; ++y) {
    for (int x = 0; x < W; ++x) {
      int dx = x - radius, dy = y - radius;
      if (dx * dx + dy * dy > radiusSquared) roundedFrame[y * W + x] = 0;
    }
  }
  SDL_UpdateTexture(texture, nullptr, roundedFrame, W * sizeof(uint16_t));
  SDL_SetRenderDrawColor(renderer, 8, 12, 19, 255);
  SDL_RenderClear(renderer);
  const SDL_Rect rect{BORDER, BORDER, W, H};
  SDL_RenderCopy(renderer, texture, nullptr, &rect);
  SDL_RenderPresent(renderer);
  dirty = false;
}
} // namespace

int main(int argc, char **argv) {
  bool smoke = argc > 1 && std::string(argv[1]) == "--smoke-test";
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
    std::fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
    return 1;
  }
  SDL_Window *window = SDL_CreateWindow(
      "MotoNav · LVGL 8.3 Simulator (MOCK DATA)",
      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, PANEL, PANEL, SDL_WINDOW_SHOWN);
  if (!window) { std::fprintf(stderr, "SDL window: %s\n", SDL_GetError()); SDL_Quit(); return 1; }
  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
  SDL_Texture *texture = renderer ? SDL_CreateTexture(
      renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, W, H) : nullptr;
  if (!texture) { std::fprintf(stderr, "SDL renderer: %s\n", SDL_GetError()); SDL_DestroyWindow(window); SDL_Quit(); return 1; }

  lv_init();
  lv_disp_draw_buf_init(&lvDraw, renderBuffer, nullptr, W * 36);
  lv_disp_drv_init(&lvDisp);
  lvDisp.hor_res = W; lvDisp.ver_res = H;
  lvDisp.draw_buf = &lvDraw; lvDisp.flush_cb = flush;
  lv_disp_drv_register(&lvDisp);
  lv_indev_drv_init(&lvPointer);
  lvPointer.type = LV_INDEV_TYPE_POINTER; lvPointer.read_cb = readPointer;
  lv_indev_drv_register(&lvPointer);
  showPage(0);

  bool running = true;
  uint32_t lastTick = SDL_GetTicks();
  int frames = 0;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) running = false;
      if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_ESCAPE) running = false;
        if (event.key.keysym.sym == SDLK_LEFT) showPage(pageIndex - 1);
        if (event.key.keysym.sym == SDLK_RIGHT) showPage(pageIndex + 1);
        if (event.key.keysym.sym == SDLK_n) { navigation = !navigation; showPage(0); }
        if (event.key.keysym.sym == SDLK_b) { linkReady = !linkReady; showPage(pageIndex); }
        if (event.key.keysym.sym == SDLK_UP) { speed = std::min(180, speed + 5); showPage(pageIndex); }
        if (event.key.keysym.sym == SDLK_DOWN) { speed = std::max(0, speed - 5); showPage(pageIndex); }
      }
      if (event.type == SDL_MOUSEWHEEL) showPage(pageIndex + (event.wheel.y < 0 ? 1 : -1));
      if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        pressed = true; dragging = true;
        downX = event.button.x - BORDER; downY = event.button.y - BORDER;
      }
      if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        pressed = false;
        const int x = event.button.x - BORDER, y = event.button.y - BORDER;
        if (dragging && std::abs(x - downX) > 56) showPage(pageIndex + (x < downX ? 1 : -1));
        else if (dragging && y - downY > 56) showPage(0);
        dragging = false;
      }
      if (event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONDOWN) {
        int mx, my; SDL_GetMouseState(&mx, &my);
        cursorX = std::max(0, std::min(W - 1, mx - BORDER));
        cursorY = std::max(0, std::min(H - 1, my - BORDER));
      }
    }
    uint32_t now = SDL_GetTicks();
    uint32_t elapsed = now - lastTick;
    if (elapsed) { lv_tick_inc(elapsed); lastTick = now; }
    lv_timer_handler();
    if (dirty || frames < 3) render(renderer, texture);
    if (smoke && ++frames >= 12) running = false;
    SDL_Delay(8);
  }
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
