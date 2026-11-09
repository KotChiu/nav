#include <cassert>
#include "../boards/21/MotoNav/ui/PageSlideBlit.h"

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

static void verifyPaddedBlit(bool horizontal, int direction) {
  constexpr int width = 7, height = 5, stride = 11, border = 2;
  uint16_t oldFrame[width * height], newFrame[width * height];
  uint16_t target[stride * (height + 2 * border)];
  for (int i = 0; i < width * height; ++i) {
    oldFrame[i] = 10 + i;
    newFrame[i] = 100 + i;
  }
  const int extent = horizontal ? width : height;
  for (int step = 0; step <= extent; ++step) {
    for (auto &pixel : target) pixel = 0xFFFF;
    const auto positions = motonavSlidePositions(step, horizontal, direction, width, height);
    motonavBlitPageSlide(target + border * stride + border, stride, oldFrame,
                        newFrame, width, height, positions);
    for (int y = 0; y < height + 2 * border; ++y) {
      for (int x = 0; x < stride; ++x) {
        const int vx = x - border, vy = y - border;
        uint16_t expected = 0xFFFF;
        if (vx >= 0 && vx < width && vy >= 0 && vy < height) {
          const int shifted = (horizontal ? vx : vy) - direction * step;
          const bool fromOld = shifted >= 0 && shifted < extent;
          const int sample = (shifted + extent) % extent;
          expected = (fromOld ? oldFrame : newFrame)
                    [horizontal ? vy * width + sample : sample * width + vx];
        }
        assert(target[y * stride + x] == expected);
      }
    }
  }
}

int main() {
  verifyPaddedBlit(true, +1);
  verifyPaddedBlit(true, -1);
  verifyPaddedBlit(false, +1);
  verifyPaddedBlit(false, -1);
  verifyAxis(true, +1);
  verifyAxis(true, -1);
  verifyAxis(false, +1);
  verifyAxis(false, -1);
  auto neutral = motonavSlidePositions(20, true, 0, 466, 466);
  assert(neutral.oldX == 0 && neutral.newX == 0);
  return 0;
}
