#pragma once
// Pure, host-testable layout and occlusion policy for the 2.1" navigation HUD.
// Keep informative controls inside the 466px logical circle. Safety content
// (lanes / speed limits) always wins over decorative connectivity chips.
#include <cstdint>

namespace MotoNavUI {
struct NavChromeRect { int x, y, w, h; };
static constexpr NavChromeRect NavSpeedCard{175,31,116,68};
static constexpr NavChromeRect NavSpeedCardWithCamera{135,31,116,68};
static constexpr NavChromeRect NavTurnCard{96,337,274,80};
static constexpr NavChromeRect NavDistanceCard{243,341,124,58};
static constexpr NavChromeRect NavMapChip{58,99,109,35};
static constexpr NavChromeRect NavLinkChip{304,99,104,35};

static inline bool navChromeFits(const NavChromeRect &r,
                                 int center=233, int radius=232) {
  if (r.w <= 0 || r.h <= 0) return false;
  const int xs[2]={r.x,r.x+r.w}, ys[2]={r.y,r.y+r.h};
  const int64_t rr=int64_t(radius)*radius;
  for (int x : xs) for (int y : ys) {
    const int64_t dx=x-center, dy=y-center;
    if (dx*dx+dy*dy>rr) return false;
  }
  return true;
}

static inline bool navChromeOverlap(const NavChromeRect &a,
                                    const NavChromeRect &b) {
  return a.x < b.x+b.w && a.x+a.w > b.x &&
         a.y < b.y+b.h && a.y+a.h > b.y;
}

// Map status indicators provide extra information only. Never compete with
// turn-lane instructions or the camera / speed-limit HUD. Minimal map hides
// these decorative status indicators without hiding essential navigation.
static inline bool navChromeShowStatus(bool liveMap, bool cleanMap,
                                       bool lanesVisible, bool cameraVisible) {
  return liveMap && !cleanMap && !lanesVisible && !cameraVisible;
}
} // namespace MotoNavUI
