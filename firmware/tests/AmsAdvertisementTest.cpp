#include "../boards/21/MotoNav/src/connectivity/AmsAdvertisement.h"

#include <cstdint>
#include <iostream>

int main() {
  if (sizeof(AmsAdvertisement::Solicited) != 18) {
    std::cerr << "wrong solicitation length\n";
    return 1;
  }
  if (AmsAdvertisement::Solicited[0] != 0x11 || AmsAdvertisement::Solicited[1] != 0x15) {
    std::cerr << "wrong AD length/type\n";
    return 1;
  }
  const uint8_t canonical[16] = {
    0x89, 0xD3, 0x50, 0x2B, 0x0F, 0x36, 0x43, 0x3A,
    0x8E, 0xF4, 0xC5, 0x02, 0xAD, 0x55, 0xF8, 0xDC
  };
  for (int i = 0; i < 16; ++i) {
    if (AmsAdvertisement::Solicited[2 + i] != canonical[15 - i]) {
      std::cerr << "AMS UUID is not little-endian\n";
      return 1;
    }
  }
  std::cout << "AMS advertisement: 3 checks passed\n";
  return 0;
}
