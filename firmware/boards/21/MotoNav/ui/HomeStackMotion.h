#pragma once
// Pure interaction and geometry policy for MotoNav's real Home card stack.
// No LVGL/Arduino dependencies: host tests exercise every intermediate pose.
#include <cmath>
#include <cstdint>

namespace MotoNavUI {
struct HomeStackRect { int x,y,w,h; };
enum HomeStackSlot : int { StackMarkets=0, StackRide=1, StackClock=2 };

static constexpr HomeStackRect StackClosed[3]={
  {243,345,140,64},  // Markets: 49px visible beneath the main card
  {82,345,145,64},   // Ride: 49px visible beneath the main card
  {67,115,332,245}
};
static constexpr HomeStackRect StackOpen[3]={
  {111,331,244,84},  // Markets, lower full card
  {85,239,296,86},   // Ride, upper full card
  {73,78,320,150}    // Compact clock summary
};
static constexpr int StackCenter=233;
static constexpr int StackRadius=232;
static constexpr float StackGestureTravel=192.0f;

static inline float stackClamp(float value) {
  return value<0?0:value>1?1:value;
}
static inline HomeStackRect stackRect(int slot,float progress) {
  if (slot<0 || slot>2) return {0,0,0,0};
  const HomeStackRect &a=StackClosed[slot],&b=StackOpen[slot];
  const float t=stackClamp(progress);
  return {
    (int)lroundf(a.x+(b.x-a.x)*t),
    (int)lroundf(a.y+(b.y-a.y)*t),
    (int)lroundf(a.w+(b.w-a.w)*t),
    (int)lroundf(a.h+(b.h-a.h)*t)
  };
}
static inline bool stackInsideCircle(const HomeStackRect &r) {
  if(r.w<=0||r.h<=0)return false;
  const int64_t radiusSq=int64_t(StackRadius)*StackRadius;
  const int xs[]={r.x,r.x+r.w},ys[]={r.y,r.y+r.h};
  for(int x:xs)for(int y:ys){
    const int64_t dx=x-StackCenter,dy=y-StackCenter;
    if(dx*dx+dy*dy>radiusSq)return false;
  }
  return true;
}
static inline bool stackGestureEnabled(bool home,bool navigation,bool modal,
                                       bool pageSwipe,bool controlCaptured) {
  return home && !navigation && !modal && !pageSwipe && !controlCaptured;
}
static inline bool stackTouchInArea(int x,int y) {
  // Include the main clock and both peeking tabs, but not the system edge.
  return x>=65 && x<=400 && y>=78 && y<=420;
}
static inline float stackDragProgress(float starting,int logicalDeltaY) {
  // Positive y is a downward finger movement: down collapses, up expands.
  return stackClamp(starting-(float)logicalDeltaY/StackGestureTravel);
}
static inline float stackReleaseTarget(float progress,float started,int deltaY) {
  // A meaningful deliberate flick must work even if it covers <50% travel.
  if(started<0.5f && deltaY<=-60)return 1.0f;
  if(started>=0.5f && deltaY>=60)return 0.0f;
  return progress>=0.5f?1.0f:0.0f;
}
} // namespace MotoNavUI
