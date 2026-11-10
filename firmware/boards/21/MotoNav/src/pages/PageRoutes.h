#pragma once

// Clock information is part of Home. The remaining pages form one horizontal
// strip. The sign is the direction of the finger in logical canvas coordinates
// (+right / +down).
enum MotoNavPage : int {
  PAGE_HOME = 0,
  PAGE_WEATHER = 1,
  PAGE_MUSIC = 2,
  PAGE_SETTINGS = 3,
  PAGE_COMPANION = 4,
  PAGE_STOCKS = 5,
  PAGE_PHOTO = 6,
};

inline int pageForSwipe(int current, bool horizontal, int sign) {
  const int order[] = {PAGE_HOME, PAGE_WEATHER, PAGE_MUSIC,
                       PAGE_STOCKS, PAGE_SETTINGS, PAGE_PHOTO};
  if (!sign) return -1;
  if (!horizontal) return sign > 0 && current != PAGE_HOME ? PAGE_HOME : -1;
  for (int i = 0; i < 6; ++i) {
    if (order[i] != current) continue;
    int next = i + (sign < 0 ? 1 : -1);
    return next >= 0 && next < 6 ? order[next] : -1;
  }
  return -1;
}
