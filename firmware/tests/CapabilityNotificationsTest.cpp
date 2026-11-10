#include "../boards/21/MotoNav/src/protocol/CapabilityNotifications.h"

#include <cstdint>
#include <iostream>

int main() {
  uint8_t pending = CapabilityNotifications::All;
  for (int expected = 0; expected < CapabilityNotifications::Count; ++expected) {
    const int actual = CapabilityNotifications::take(pending);
    if (actual != expected) {
      std::cerr << "expected capability " << expected << ", got " << actual << "\n";
      return 1;
    }
  }
  if (pending != 0 || CapabilityNotifications::take(pending) != -1) {
    std::cerr << "empty queue must stay empty\n";
    return 1;
  }

  pending = 1 << 4;
  if (CapabilityNotifications::take(pending) != 4 || pending != 0) {
    std::cerr << "single capability must clear only its bit\n";
    return 1;
  }
  std::cout << "Capability notification scheduling: 2 checks passed\n";
  return 0;
}
