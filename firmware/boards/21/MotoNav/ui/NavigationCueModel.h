#pragma once
// Pure navigation cue classification (shared across ESP32 and desktop tests).
// Unknown instructions never render an invented turn.
#include <cstring>
enum class MotoNavCueKind { Straight, Left, Right, SlightLeft, SlightRight, UTurn, Arrive, Unknown };
static inline MotoNavCueKind motonavCueKind(const char *direction) {
  if (!direction) return MotoNavCueKind::Unknown;
  if (std::strcmp(direction,"S")==0) return MotoNavCueKind::Straight;
  if (std::strcmp(direction,"L")==0) return MotoNavCueKind::Left;
  if (std::strcmp(direction,"R")==0) return MotoNavCueKind::Right;
  if (std::strcmp(direction,"SL")==0) return MotoNavCueKind::SlightLeft;
  if (std::strcmp(direction,"SR")==0) return MotoNavCueKind::SlightRight;
  if (std::strcmp(direction,"U")==0) return MotoNavCueKind::UTurn;
  if (std::strcmp(direction,"A")==0) return MotoNavCueKind::Arrive;
  return MotoNavCueKind::Unknown;
}
static inline bool motonavCueSupported(MotoNavCueKind kind) {
  return kind != MotoNavCueKind::Unknown;
}
static inline float motonavCueDefaultAngle(MotoNavCueKind kind) {
  switch(kind) {
    case MotoNavCueKind::Left: return -90.f;
    case MotoNavCueKind::Right: return 90.f;
    case MotoNavCueKind::SlightLeft: return -45.f;
    case MotoNavCueKind::SlightRight: return 45.f;
    default: return 0.f;
  }
}
