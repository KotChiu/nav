#include <cassert>
#include <cmath>
#include <initializer_list>
#include "../boards/21/MotoNav/ui/HomeStackMotion.h"

using namespace MotoNavUI;

int main(){
  // The entire gesture path, not just its endpoints, must stay on the round
  // 466x466 canvas. Check pixel-rounded intermediate positions.
  for(int p=0;p<=100;p++) for(int slot=0;slot<3;slot++)
    assert(stackInsideCircle(stackRect(slot,p/100.0f)));
  assert(!stackInsideCircle({10,10,450,450}));
  assert(!stackInsideCircle({0,0,466,466}));
  const HomeStackRect closedMain=stackRect(StackClock,0);
  const HomeStackRect closedRide=stackRect(StackRide,0);
  const HomeStackRect closedMarkets=stackRect(StackMarkets,0);
  assert(closedRide.y+closedRide.h-(closedMain.y+closedMain.h)>=44);
  assert(closedMarkets.y+closedMarkets.h-(closedMain.y+closedMain.h)>=44);

  // When opened, the two cards have independent large hit areas and no
  // overlap with one another or the compact clock summary.
  const HomeStackRect openMain=stackRect(StackClock,1);
  const HomeStackRect openRide=stackRect(StackRide,1);
  const HomeStackRect openMarkets=stackRect(StackMarkets,1);
  assert(openMain.y+openMain.h<openRide.y);
  assert(openRide.y+openRide.h<openMarkets.y);
  assert(openRide.h>=80 && openMarkets.h>=80);
  assert(openRide.w>=240 && openMarkets.w>=240);

  assert(stackGestureEnabled(true,false,false,false,false));
  assert(!stackGestureEnabled(true,true,false,false,false));
  assert(!stackGestureEnabled(true,false,true,false,false));
  assert(!stackGestureEnabled(true,false,false,true,false));
  assert(!stackGestureEnabled(true,false,false,false,true));
  assert(!stackGestureEnabled(false,false,false,false,false));
  assert(stackTouchInArea(233,233));
  assert(stackTouchInArea(233,395));
  assert(!stackTouchInArea(233,20));

  assert(stackDragProgress(0,-96)>0.49f);
  assert(stackDragProgress(1,96)<0.51f);
  assert(stackDragProgress(0,100)==0);
  assert(stackDragProgress(1,-100)==1);
  assert(stackReleaseTarget(.35f,0,-68)==1);
  assert(stackReleaseTarget(.90f,1,75)==0);
  assert(stackReleaseTarget(.20f,0,-20)==0);
  assert(stackReleaseTarget(.80f,1,20)==1);
  return 0;
}
