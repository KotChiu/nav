#include <array>
#include <cassert>
#include <cstdint>
#include "../boards/21/MotoNav/ui/Rgb565Plane.h"

static constexpr int W = 9, H = 7;
static void verifyGuards(const std::array<uint16_t, W * H + 2> &v) {
  assert(v.front() == 0xCAFE && v.back() == 0xBEEF);
}

int main() {
  std::array<uint16_t, W * H + 2> storage{};
  storage.front() = 0xCAFE;
  storage.back() = 0xBEEF;
  MotoNavRgb565Plane p{storage.data() + 1, W, H};
  p.rect(-10, -10, 25, 25, 0x1111);
  for (int y = 0; y < H; ++y)
    for (int x = 0; x < W; ++x)
      assert(storage[1 + y * W + x] == 0x1111);
  verifyGuards(storage);
  p.rect(2, 1, 3, 2, 0x2222);
  for (int y = 0; y < H; ++y)
    for (int x = 0; x < W; ++x)
      assert(storage[1 + y * W + x] ==
             ((x >= 2 && x < 5 && y >= 1 && y < 3) ? 0x2222 : 0x1111));
  p.hline(4, 3, -3, 0x3333); // x=2,3,4
  for (int x = 0; x < W; ++x)
    assert(storage[1 + 3 * W + x] == ((x >= 2 && x <= 4) ? 0x3333 : 0x1111));
  p.vline(4, 5, -4, 0x4444); // y=2,3,4,5
  for (int y = 0; y < H; ++y) {
    uint16_t want = y >= 2 && y <= 5 ? 0x4444 : 0x1111;
    assert(storage[1 + y * W + 4] == want);
  }
  p.pixel(8, 6, 0x5555);
  assert(storage[1 + 6 * W + 8] == 0x5555);
  p.pixel(-1, -1, 0xFFFF);
  p.hline(-100, 100, 40, 0xFFFF);
  p.vline(100, -100, 4, 0xFFFF);
  p.rect(5, 5, -1, 2, 0xFFFF);
  verifyGuards(storage);
  MotoNavRgb565Plane empty{};
  empty.pixel(0, 0, 0xFFFF);
  empty.rect(0, 0, 2, 2, 0xFFFF);
  return 0;
}
