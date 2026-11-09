#pragma once
// MotoNav electronic badge protocol, shared by unit tests and ESP32.
// BLE bulk type 6, payload = "BDG1" + LE16(width) + LE16(height)
// + LE32(CRC32 of RGB565 little-endian pixels) + raw pixel bytes.
#include <cstddef>
#include <cstdint>

namespace MotoNavBadge {
constexpr uint16_t SIZE = 240;
constexpr size_t PIXEL_BYTES = size_t(SIZE) * SIZE * 2;
constexpr size_t HEADER_BYTES = 12;
constexpr size_t PACKET_BYTES = HEADER_BYTES + PIXEL_BYTES;
constexpr uint8_t BULK_TYPE = 6;

static inline uint16_t le16(const uint8_t *p) {
  return uint16_t(p[0]) | (uint16_t(p[1]) << 8);
}
static inline uint32_t le32(const uint8_t *p) {
  return uint32_t(p[0]) | (uint32_t(p[1]) << 8) |
         (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}
static inline uint32_t crc32(const uint8_t *bytes, size_t n) {
  uint32_t crc = 0xFFFFFFFFu;
  for (size_t i = 0; i < n; ++i) {
    crc ^= bytes[i];
    for (int bit = 0; bit < 8; ++bit)
      crc = (crc >> 1) ^ ((crc & 1u) ? 0xEDB88320u : 0u);
  }
  return ~crc;
}
static inline bool valid(const uint8_t *packet, size_t n) {
  return packet && n == PACKET_BYTES &&
      packet[0] == 'B' && packet[1] == 'D' &&
      packet[2] == 'G' && packet[3] == '1' &&
      le16(packet + 4) == SIZE && le16(packet + 6) == SIZE &&
      le32(packet + 8) == crc32(packet + HEADER_BYTES, PIXEL_BYTES);
}
} // namespace MotoNavBadge
