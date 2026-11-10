#include <cassert>
#include <initializer_list>
#include "../boards/21/MotoNav/ui/CircularCardGeometry.h"

int main() {
  const MotoNavCardRect focusedHome{67,115,332,245};
  const MotoNavCardRect homeBack1{82,345,145,64};  // Ride
  const MotoNavCardRect homeBack2{243,345,140,64}; // Markets
  const MotoNavCardRect stockFront{73,139,320,218};
  const MotoNavCardRect stockBack1{86,345,294,49};
  const MotoNavCardRect stockBack2{106,362,254,50};
  const MotoNavCardRect settingsFront{53,176,360,202};
  const MotoNavCardRect settingsBack1{75,184,316,213};
  const MotoNavCardRect settingsBack2{98,194,270,216};
  const MotoNavCardRect mapLeftChip{58,99,109,35};
  const MotoNavCardRect mapRightChip{304,99,104,35};
  for (const auto &r : {focusedHome,homeBack1,homeBack2,stockFront,
                        stockBack1,stockBack2,settingsFront,settingsBack1,
                        settingsBack2,mapLeftChip,mapRightChip})
    assert(motonavInsideCircle(r));
  assert(!motonavInsideCircle({10,10,340,340}));
  assert(!motonavInsideCircle({0,0,466,466}));
  // Both exposed action areas remain >=44px tall, unlike the old 18px
  // bottom-most sheet. This is not a hardware hit-test with riding gloves.
  const int homeFrontBottom = focusedHome.y + focusedHome.height;
  assert(homeBack1.y + homeBack1.height - homeFrontBottom >= 44);
  assert(homeBack2.y + homeBack2.height - homeFrontBottom >= 44);
  assert(homeBack1.x + homeBack1.width < homeBack2.x);
  // The home primary card has a deliberate three-step hierarchy: quiet
  // date/connection, one large 64px clock, one compact weather row. No
  // inner widget may escape the focused card or cover the peeking tabs.
  const MotoNavCardRect homeLocal{0,0,332,245};
  assert(motonavWithinCard({25,28,177,25},homeLocal)); // date
  assert(motonavWithinCard({19,60,295,90},homeLocal)); // large time
  assert(motonavWithinCard({207,19,116,34},homeLocal)); // link dot
  assert(motonavWithinCard({25,161,282,1},homeLocal)); // divider
  assert(motonavWithinCard({18,174,296,62},homeLocal)); // weather
  // Do not move weather onto the main time field on future iterations.
  assert(60 + 90 < 161);
  assert(161 < 174);

  // All quote content stays within one 320x218 focused card.
  const MotoNavCardRect stockLocal{0,0,320,218};
  assert(motonavWithinCard({19,16,164,28},stockLocal));
  assert(motonavWithinCard({20,49,279,37},stockLocal));
  assert(motonavWithinCard({21,88,278,27},stockLocal));
  assert(motonavWithinCard({20,123,280,47},stockLocal));
  assert(!motonavWithinCard({100,100,306,52},stockLocal));
  // Round-screen content at the bottom is intentionally narrower.
  assert(!motonavInsideCircle({58,354,344,75}));
  return 0;
}
