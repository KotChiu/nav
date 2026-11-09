#include <cassert>
#include "../boards/21/MotoNav/src/map/PlaceLabels.h"

int main() {
  PlaceLabels labels;
  uint8_t valid[] = {'P', 1, 0x34, 0x12, 1,
    0xfe, 0xff, 0xff, 0xff, 23, 0, 0, 0, 3, 1, 0xf0, 0xb0};
  uint8_t *buffer = valid;
  assert(labels.replace(buffer, sizeof(valid)));
  assert(buffer == nullptr && labels.frame == 0x1234 && labels.count == 1);
  assert(labels.labels[0].x == -2 && labels.labels[0].y == 23);
  assert(labels.labels[0].width == 3 && labels.labels[0].pixels[1] == 0xb0);

  uint8_t malformed[] = {'P', 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 255, 1};
  buffer = malformed;
  assert(!labels.replace(buffer, sizeof(malformed)));
  assert(labels.count == 1 && labels.storage == valid && buffer == malformed);

  uint8_t empty[] = {'P', 1, 0x34, 0x12, 0};
  buffer = empty;
  assert(labels.replace(buffer, sizeof(empty)));
  assert(labels.count == 0 && buffer == valid);

  // Five pixels span two bytes. Padding bits must not leak into the next row.
  uint8_t large[] = {'P', 2, 0x34, 0x12, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 5, 2, 0x1b, 0xc0, 0xe4, 0x40};
  buffer = large;
  assert(!labels.replace(buffer, sizeof(large) - 1));
  assert(labels.storage == empty);
  assert(labels.replace(buffer, sizeof(large)));
  assert(labels.version == 2 && labels.count == 1 && buffer == empty);
  const uint8_t *glyph = labels.labels[0].pixels;
  assert(PlaceLabels::stride(5, 2) == 2);
  assert(PlaceLabels::coverage(glyph, 0, 2) == 0);
  assert(PlaceLabels::coverage(glyph, 1, 2) == 5);
  assert(PlaceLabels::coverage(glyph, 2, 2) == 10);
  assert(PlaceLabels::coverage(glyph, 3, 2) == 15);
  assert(PlaceLabels::coverage(glyph, 4, 2) == 15);
  assert(PlaceLabels::coverage(glyph + 2, 4, 2) == 5);
  assert(PlaceLabels::coverage(valid + 15, 0, 1) == 15);
  assert(PlaceLabels::coverage(valid + 15, 2, 1) == 11);


  assert(PlaceLabels::fragment(517, 3, 0, 0, 250, 250));
  assert(PlaceLabels::fragment(517, 3, 2, 500, 17, 250));
  assert(!PlaceLabels::fragment(517, 3, 2, 499, 18, 250));
  assert(!PlaceLabels::fragment(517, 3, 1, 250, 249, 250));
  assert(!PlaceLabels::fragment(517, 2, 1, 250, 250, 250));
  assert(!PlaceLabels::fragment(517, 3, 3, 750, 17, 250));
}
