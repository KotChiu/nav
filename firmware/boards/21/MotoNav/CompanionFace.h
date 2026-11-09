#pragma once
#include <stdint.h>

// Small, independent eye poses. No full-screen animation buffers are needed.
namespace CompanionFace {
enum Expression : uint8_t { Calm, Happy, Wink, Surprised, Curious, Sleepy, Count };
struct Eye { int16_t x, y, w, h; };
struct Pose { Eye eye[2]; const char *name; };
inline Expression next(Expression expression) {
  return static_cast<Expression>((static_cast<uint8_t>(expression) + 1) % Count);
}
inline Pose pose(Expression expression, bool blink, bool lookRight) {
  Pose out = {{{37,25,28,50}, {95,25,28,50}}, "CALM"};
  switch (expression) {
    case Happy: out = {{{32,37,36,18},{92,37,36,18}}, "HAPPY"}; break;
    case Wink: out = {{{33,44,35,6},{95,24,28,52}}, "WINK"}; break;
    case Surprised: out = {{{31,18,36,64},{93,18,36,64}}, "WOW!"}; break;
    case Curious: out = {{{33,18,32,58},{96,33,24,34}}, "CURIOUS"}; break;
    case Sleepy: out = {{{32,46,36,5},{92,46,36,5}}, "SLEEPY"}; break;
    default: break;
  }
  for (int i=0;i<2;i++) {
    if (blink && expression != Sleepy) { out.eye[i].y=46; out.eye[i].h=5; }
    if (lookRight && expression != Sleepy) out.eye[i].x += 5;
  }
  return out;
}
}
