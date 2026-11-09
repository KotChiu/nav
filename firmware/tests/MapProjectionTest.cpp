#include <cassert>
#include <cmath>
#include "../boards/21/MotoNav/ui/MapProjection.h"

static void close(float a, float b) { assert(std::fabs(a - b) < 0.0001f); }
int main() {
  float x, y;
  motonavProjectDelta(0, 0, 1, 1, 0, 233, 262, x, y);
  close(x, 233); close(y, 262);
  motonavProjectDelta(50, -30, 1, 1, 0, 233, 262, x, y);
  close(x, 283); close(y, 292);
  // 90 degrees positive heading rotates east towards north on the display.
  motonavProjectDelta(10, 0, 2, 0, 1, 233, 262, x, y);
  close(x, 233); close(y, 242);
  motonavProjectDelta(0, 10, 2, 0, 1, 233, 262, x, y);
  close(x, 213); close(y, 262);
  motonavProjectWorld(1010, 1990, 1000, 2000, 1, 1, 0, 233, 262, x, y);
  close(x, 243); close(y, 272);
  // Preserve the Canvas transform exactly for all expected headings/speeds.
  for (int d = -150; d <= 150; d += 15) {
    for (int heading = 0; heading < 360; heading += 15) {
      const float cs = std::cos(heading * 3.14159265358979323846f / 180);
      const float sn = std::sin(heading * 3.14159265358979323846f / 180);
      const float dx = float(d), dy = float(d / 2);
      motonavProjectDelta(dx, dy, 0.72f, cs, sn, 233, 262, x, y);
      close(x, 233 + 0.72f * (dx * cs - dy * sn));
      close(y, 262 - 0.72f * (dx * sn + dy * cs));
    }
  }
  return 0;
}
