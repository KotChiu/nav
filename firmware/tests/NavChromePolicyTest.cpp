#include <cassert>
#include "../boards/21/MotoNav/ui/NavChromePolicy.h"

using namespace MotoNavUI;

int main() {
  for (const NavChromeRect r : {
       NavSpeedCard, NavSpeedCardWithCamera,
       NavTurnCard, NavMapChip, NavLinkChip})
    assert(navChromeFits(r));
  assert(!navChromeFits({10,10,400,400}));
  assert(!navChromeFits({0,0,466,466}));

  // Speed has dedicated space and never obscures status chips. With a
  // camera there is a different speed placement, and the chips are hidden.
  assert(!navChromeOverlap(NavSpeedCard,NavMapChip));
  assert(!navChromeOverlap(NavSpeedCard,NavLinkChip));
  assert(!navChromeOverlap(NavSpeedCardWithCamera,NavMapChip));
  assert(!navChromeOverlap(NavSpeedCardWithCamera,NavLinkChip));
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
