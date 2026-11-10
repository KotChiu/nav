#pragma once
#include <cstdint>

namespace CapabilityNotifications {
constexpr int Count = 7;
constexpr uint8_t All = (1 << Count) - 1;

inline uint8_t &queue() {
  static uint8_t pending = 0;
  return pending;
}

inline int take(uint8_t &pending) {
  for (int i = 0; i < Count; ++i) {
    const uint8_t bit = 1 << i;
    if (pending & bit) {
      pending &= ~bit;
      return i;
    }
  }
  return -1;
}
}
