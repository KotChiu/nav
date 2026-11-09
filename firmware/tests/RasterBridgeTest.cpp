// Native host regression tests for ESP32/SDL shared image conversion.
// g++ -std=c++17 -Wall -Wextra -pedantic firmware/tests/RasterBridgeTest.cpp -o /tmp/raster-test && /tmp/raster-test
#include <cassert>
#include <cstdint>
#include <vector>
#include "../boards/21/MotoNav/ui/RasterBridge.h"

int main() {
  // Invalid geometry and impossible images must not create raster objects.
  assert(!motonavBitmapBox(0, 20, 56).valid());
  assert(!motonavBitmapBox(4200, 20, 56).valid());
  assert(!motonavBitmapBox(20, 20, 500).valid());

  // BLE format is 4-bit grayscale, even pixel = high nibble.
  const uint8_t packed[] = {0x0F, 0x84};
  std::vector<uint8_t> output(4);
  assert(motonavRasterTextGray(packed, sizeof(packed), 2, 2, 2, 2, 255,
    [&](int x, int y, uint8_t gray) { output[y * 2 + x] = gray; }));
  assert(output[0] == 0 && output[1] == 255);
  assert(output[2] == 136 && output[3] == 68);
  assert(!motonavRasterTextGray(packed, 1, 2, 2, 2, 2, 255,
    [](int, int, uint8_t) {}));

  // Upscale and downscale should stay within 0..255 and respect tint.
  std::vector<uint8_t> scaled(16);
  assert(motonavRasterTextGray(packed, sizeof(packed), 2, 2, 4, 4, 128,
    [&](int x, int y, uint8_t gray) { scaled[y * 4 + x] = gray; }));
  for (auto gray : scaled) assert(gray <= 128);
  assert(scaled[0] == 0 && scaled[3] == 128);

  // Album art uses source RGB565 without modifying it; corner alpha is zero.
  const uint16_t art[] = {0xF800, 0x07E0, 0x001F, 0xFFFF};
  int visible = 0, transparent = 0;
  assert(motonavRasterCover(art, 2, 2, 8,
    [&](int x, int y, uint16_t color, bool inside) {
      if (inside) { ++visible; assert(color == art[(y * 2 / 8) * 2 + (x * 2 / 8)]); }
      else { ++transparent; assert(color == 0); }
    }));
  assert(visible > 0 && transparent > 0);
  assert(!motonavRasterCover(art, 129, 2, 8,
    [](int, int, uint16_t, bool) {}));
  return 0;
}
