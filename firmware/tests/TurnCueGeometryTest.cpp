#include <cassert>
#include <cmath>
#include "../boards/21/MotoNav/ui/TurnCueGeometry.h"
static void near(float a, float b) { assert(std::fabs(a-b) < 1e-4f); }
int main() {
  const auto up = motonavTurnArrowGeometry(0, 185, 417);
  near(up.endX, 185); near(up.endY, 377);
  assert(up.leftX < up.endX && up.rightX > up.endX);
  const auto right = motonavTurnArrowGeometry(90, 185, 417);
  near(right.endX, 225); near(right.endY, 417);
  const auto left = motonavTurnArrowGeometry(-90, 185, 417);
  near(left.endX, 145); near(left.endY, 417);
  // The two arrowhead wings must be symmetric about the centerline.
  near(up.leftY, up.rightY);
  near(right.leftX, right.rightX);
  near(left.leftX, left.rightX);
}
