#include <cassert>
#include <initializer_list>
#include "../boards/21/MotoNav/ui/NavChromePolicy.h"

using namespace MotoNavUI;

int main() {
  for (const NavChromeRect r : {
       NavSpeedCard, NavSpeedCardWithCamera,
       NavTurnCard, NavDistanceCard, NavMapChip, NavLinkChip})
    assert(navChromeFits(r));
  assert(!navChromeFits({10,10,400,400}));
  assert(!navChromeFits({0,0,466,466}));

  // Speed has dedicated space and never obscures status chips. With a
  // camera there is a different speed placement, and the chips are hidden.
  assert(!navChromeOverlap(NavSpeedCard,NavMapChip));
  assert(!navChromeOverlap(NavSpeedCard,NavLinkChip));
  assert(!navChromeOverlap(NavSpeedCardWithCamera,NavMapChip));
  assert(!navChromeOverlap(NavSpeedCardWithCamera,NavLinkChip));
  assert(navChromeOverlap(NavTurnCard,NavDistanceCard));
  assert(NavDistanceCard.x >= NavTurnCard.x && NavDistanceCard.y >= NavTurnCard.y);
  assert(NavDistanceCard.x + NavDistanceCard.w <= NavTurnCard.x + NavTurnCard.w);
  assert(NavDistanceCard.y + NavDistanceCard.h <= NavTurnCard.y + NavTurnCard.h);
  // ETA starts at y=404; the distance widget must leave it visible.
  assert(NavDistanceCard.y + NavDistanceCard.h < 404);
  assert(!navChromeOverlap(NavTurnCard,NavMapChip));
  assert(!navChromeOverlap(NavTurnCard,NavLinkChip));

  assert(navChromeShowStatus(true,false,false,false));
  assert(!navChromeShowStatus(false,false,false,false));
  assert(!navChromeShowStatus(true,true,false,false));
  assert(!navChromeShowStatus(true,false,true,false));
  assert(!navChromeShowStatus(true,false,false,true));
  assert(!navChromeShowStatus(true,false,true,true));
  return 0;
}
