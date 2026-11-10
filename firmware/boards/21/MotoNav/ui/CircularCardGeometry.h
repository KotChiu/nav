#pragma once
// Pure geometry contract: no interactive/content card is allowed to spill
// beyond the 466x466 safe circle. The 480px physical ring is handled by
// PortScale; here (233,233) is the logical center.
#include <cstdint>
struct MotoNavCardRect { int x, y, width, height; };
static inline bool motonavInsideCircle(const MotoNavCardRect &box,
                                       int center = 233, int radius = 232) {
  if (box.width <= 0 || box.height <= 0) return false;
  const int64_t r2 = int64_t(radius) * radius;
  const int x0=box.x, y0=box.y;
  const int x1=box.x+box.width, y1=box.y+box.height;
  const int corners[][2] = {{x0,y0},{x1,y0},{x0,y1},{x1,y1}};
  for (const auto &p : corners) {
    const int64_t dx = p[0] - center, dy = p[1] - center;
    if (dx*dx + dy*dy > r2) return false;
  }
  return true;
}
static inline bool motonavWithinCard(const MotoNavCardRect &child,
                                     const MotoNavCardRect &parent) {
  return child.width > 0 && child.height > 0 &&
         child.x >= parent.x && child.y >= parent.y &&
         child.x + child.width <= parent.x + parent.width &&
         child.y + child.height <= parent.y + parent.height;
}
