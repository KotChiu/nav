#include <cassert>
#include "../boards/21/MotoNav/ui/FrameOwnership.h"
int main() {
  uint16_t logical[4]{}, source[4]{}, transport[4]{};
  assert(motonavFrameBytes(466, 466) == size_t(466 * 466 * 2));
  assert(motonavFrameBytes(480, 480) == size_t(480 * 480 * 2));
  assert(motonavFrameBytes(0, 466) == 0);
  assert(motonavFrameBytes(466, -1) == 0);
  assert(motonavFrameBytes(200000, 200000) == 0);
  assert(motonavFramePointersIndependent(logical, source, transport, transport));
  assert(motonavFramePointersIndependent(logical, source, transport, nullptr));
  assert(!motonavFramePointersIndependent(logical, logical, transport, nullptr));
  assert(!motonavFramePointersIndependent(logical, transport, transport, nullptr));
  assert(!motonavFramePointersIndependent(logical, source, transport, source));
  assert(!motonavFramePointersIndependent(nullptr, source, transport, nullptr));
  assert(!motonavFramePointersIndependent(logical, nullptr, transport, nullptr));
}
