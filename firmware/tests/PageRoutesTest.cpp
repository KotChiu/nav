#include <cassert>
#include "../boards/21/MotoNav/src/pages/PageRoutes.h"

int main() {
  // Rows: right, left, down, up. -1 means that edge has no destination.
  const int expected[7][4] = {
    {-1, PAGE_WEATHER, -1, -1},
    {PAGE_HOME, PAGE_MUSIC, PAGE_HOME, -1},
    {PAGE_WEATHER, PAGE_COMPANION, PAGE_HOME, -1},
    {PAGE_STOCKS, PAGE_BADGE, PAGE_HOME, -1}, 
    {PAGE_MUSIC, PAGE_STOCKS, PAGE_HOME, -1},
    {PAGE_COMPANION, PAGE_SETTINGS, PAGE_HOME, -1},
    {PAGE_SETTINGS, -1, PAGE_HOME, -1}, 
  };
  for (int page = PAGE_HOME; page <= PAGE_BADGE; ++page) {
    assert(pageForSwipe(page, true, 1) == expected[page][0]);
    assert(pageForSwipe(page, true, -1) == expected[page][1]);
    assert(pageForSwipe(page, false, 1) == expected[page][2]);
    assert(pageForSwipe(page, false, -1) == expected[page][3]);
    assert(pageForSwipe(page, true, 0) == -1);
  }
}
