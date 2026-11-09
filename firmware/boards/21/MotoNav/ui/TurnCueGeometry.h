#pragma once
// Pure heading-to-arrow geometry. Positive headings rotate clockwise toward
// screen-right; north (0 degrees) is toward decreasing screen Y.
#include <cmath>
struct MotoNavTurnArrowGeometry {
  float startX, startY, elbowX, elbowY;
  float endX, endY, leftX, leftY, rightX, rightY;
};
static inline MotoNavTurnArrowGeometry motonavTurnArrowGeometry(
    float degrees, float centerX, float centerY) {
  const float heading = degrees * 3.14159265358979323846f / 180.0f;
  const float dx = std::sin(heading), dy = -std::cos(heading);
  MotoNavTurnArrowGeometry p{};
  p.startX = centerX;
  p.startY = centerY + 5;
  p.elbowX = centerX;
  p.elbowY = centerY - 8;
  p.endX = centerX + 40 * dx;
  p.endY = centerY + 40 * dy;
  // Screen coordinates use Y-down: the left normal of (dx,dy)
  // is (dy,-dx), not (-dy,dx).
  p.leftX = p.endX - 14 * dx + 10 * dy;
  p.leftY = p.endY - 14 * dy - 10 * dx;
  p.rightX = p.endX - 14 * dx - 10 * dy;
  p.rightY = p.endY - 14 * dy + 10 * dx;
  return p;
}
