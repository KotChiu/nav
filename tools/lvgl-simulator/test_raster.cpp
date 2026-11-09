#include "../../firmware/boards/21/MotoNav/ui/RasterBridge.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <vector>

int main() {
  // Exact-fit text near the center.
  const auto centered = motonavBitmapBox(100, 16, 170);
  assert(centered.valid() && centered.x == 183 && centered.width == 100);
  assert(!motonavBitmapBox(0, 16, 100).valid());
  assert(!motonavBitmapBox(100, 0, 100).valid());
  assert(!motonavBitmapBox(100, 20, 500).valid());
  assert(!motonavBitmapBox(4097, 20, 100).valid());

  // Odd widths have row padding, and high nibble is the left pixel.
  const std::array<uint8_t, 4> packed = {0xF8, 0x10, 0x24, 0xE0};
  std::array<uint8_t, 6> full = {};
  bool result = motonavRasterTextGray(
      packed.data(), packed.size(), 3, 2, 3, 2, 255,
      [&](int x, int y, uint8_t g) { full[y * 3 + x] = g; });
  assert(result);
  assert(full[0] == 255 && full[1] == 136 && full[2] == 17);
  assert(full[3] == 34 && full[4] == 68 && full[5] == 238);

  // Downscaling averages gray values instead of nearest neighbor.
  const std::array<uint8_t, 2> small = {0xF0, 0x0F}; // 2x2: 15,0 / 0,15
  uint8_t average = 0;
  result = motonavRasterTextGray(
      small.data(), small.size(), 2, 2, 1, 1, 255,
      [&](int, int, uint8_t g) { average = g; });
  assert(result && average == 136); // round(7.5)=8 => 8*17
  assert(!motonavRasterTextGray(
      packed.data(), 2, 3, 2, 3, 2, 255,
      [&](int, int, uint8_t) {})); // short packet rejected

  // RGB565 cover: center survives, circular corners become transparent.
  std::vector<uint16_t> cover(4 * 4, 0xF800);
  int opaque = 0, transparent = 0;
  result = motonavRasterCover(
      cover.data(), 4, 4, 16,
      [&](int x, int y, uint16_t pixel, bool inside) {
        if (!inside) {
          ++transparent;
          assert(pixel == 0);
          assert((x == 0 && y == 0) || x != 8 || y != 8);
        } else {
          ++opaque;
          assert(pixel == 0xF800);
        }
      });
  assert(result && opaque > 100 && transparent > 20);
  assert(!motonavRasterCover(
      cover.data(), 0, 4, 16, [&](int, int, uint16_t, bool) {}));
  assert(!motonavRasterCover(
      cover.data(), 4, 4, 129, [&](int, int, uint16_t, bool) {}));
  return 0;
}
