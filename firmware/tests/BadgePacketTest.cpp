#include <cassert>
#include <vector>
#include "../boards/21/MotoNav/ui/BadgePacket.h"
int main() {
  using namespace MotoNavBadge;
  assert(crc32(reinterpret_cast<const uint8_t *>("123456789"), 9) == 0xCBF43926u);
  std::vector<uint8_t> packet(PACKET_BYTES, 0);
  packet[0] = 'B'; packet[1] = 'D'; packet[2] = 'G'; packet[3] = '1';
  packet[4] = uint8_t(SIZE); packet[5] = uint8_t(SIZE >> 8);
  packet[6] = uint8_t(SIZE); packet[7] = uint8_t(SIZE >> 8);
  for (size_t i = HEADER_BYTES; i < packet.size(); ++i) packet[i] = uint8_t(i % 251);
  auto crc = crc32(packet.data() + HEADER_BYTES, PIXEL_BYTES);
  for (int j = 0; j < 4; ++j) packet[8+j] = uint8_t(crc >> (8*j));
  assert(valid(packet.data(), packet.size()));
  packet.back() ^= 1;
  assert(!valid(packet.data(), packet.size()));
  packet.back() ^= 1;
  packet[4] ^= 1;
  assert(!valid(packet.data(), packet.size()));
  packet[4] ^= 1;
  assert(!valid(packet.data(), packet.size()-1));
  packet[0] = 'X';
  assert(!valid(packet.data(), packet.size()));
}
