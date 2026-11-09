#include <cassert>
#include "../boards/21/MotoNav/ui/PageSlideGeometry.h"

static void verifyAxis(bool horizontal, int sign) {
  constexpr int width = 466, height = 466;
  for (int step = 0; step <= width; ++step) {
    auto p = motonavSlidePositions(step, horizontal, sign, width, height);
    int old = horizontal ? p.oldX : p.oldY;
    int incoming = horizontal ? p.newX : p.newY;
    // No 1px gap between the two full-sized source images.
    const int right = old > incoming ? old : incoming;
    const int left = old < incoming ? old : incoming;
    assert(right - left == width);
    assert(left <= 0 && right >= 0);
    assert(left + width >= 0 && right <= width);
    assert((horizontal ? p.oldY == 0 && p.newY == 0 :
                        p.oldX == 0 && p.newX == 0));
  }
  auto start = motonavSlidePositions(0, horizontal, sign, width, height);
  auto end = motonavSlidePositions(width, horizontal, sign, width, height);
  assert((horizontal ? start.oldX : start.oldY) == 0);
  assert((horizontal ? end.newX : end.newY) == 0);
  auto clamped = motonavSlidePositions(10000, horizontal, sign, width, height);
  assert((horizontal ? clamped.newX : clamped.newY) == 0);
}

int main() {
  verifyAxis(true, +1);
  verifyAxis(true, -1);
  verifyAxis(false, +1);
  verifyAxis(false, -1);
  auto neutral = motonavSlidePositions(20, true, 0, 466, 466);
  assert(neutral.oldX == 0 && neutral.newX == 0);
  return 0;
}
