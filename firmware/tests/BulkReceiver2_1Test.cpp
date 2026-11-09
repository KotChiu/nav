// Host-side regression tests for the real 2.1 BulkReceiver implementation.
// Run: g++ -std=c++17 firmware/tests/BulkReceiver2_1Test.cpp -o /tmp/bulk-21 && /tmp/bulk-21
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

struct String {
  std::string value;
  String() = default;
  String(int n) : value(std::to_string(n)) {}
  String(const char *s) : value(s) {}
  String(std::string s) : value(std::move(s)) {}
  String &operator=(const char *s) { value = s; return *this; }
  friend String operator+(String a, const String &b) { return String(a.value + b.value); }
};
struct Bulk {
  uint8_t *buf = nullptr;
  uint32_t cap = 0, total = 0, receivedBytes = 0;
  int id = -1;
  uint16_t parts = 0, got = 0;
  bool done = false;
  uint8_t mask[512] = {};
};
Bulk bulks[6];
uint8_t *mapShow = nullptr;
String pendingAck;
bool navDirty = false;
int parsedMaps = 0, parsedRoutes = 0;
bool acceptRaster = true;

int32_t rd32(const uint8_t *p) {
  return static_cast<int32_t>(
      uint32_t(p[0]) | (uint32_t(p[1]) << 8) |
      (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24));
}
void parseMap(uint8_t *, uint32_t) { ++parsedMaps; }
void parseRoute(uint8_t *, uint32_t) { ++parsedRoutes; }
void parseGhost(uint8_t *, uint32_t) {}
void parseCover(uint8_t *, uint32_t) {}
bool parseRaster(uint8_t *, uint32_t) { return acceptRaster; }
#include "../boards/21/MotoNav/src/protocol/BulkReceiver.h"

static std::vector<uint8_t> packet(int type, int id, int parts, int seq,
                                    uint32_t total, uint32_t off,
                                    std::initializer_list<uint8_t> bytes) {
  std::vector<uint8_t> p = {'M', uint8_t(type), uint8_t(id),
    uint8_t(parts), uint8_t(parts >> 8), uint8_t(seq), uint8_t(seq >> 8)};
  for (unsigned i = 0; i < 4; ++i) p.push_back(uint8_t(total >> (i * 8)));
  for (unsigned i = 0; i < 4; ++i) p.push_back(uint8_t(off >> (i * 8)));
  p.insert(p.end(), bytes.begin(), bytes.end());
  return p;
}
static void receive(const std::vector<uint8_t> &p) {
  handleBulk(p.data(), static_cast<int>(p.size()));
}
static uint8_t mapBuffer[64] = {}, routeBuffer[64] = {}, showBuffer[64] = {};
static uint8_t rasterBuffer[64] = {};
static void reset() {
  for (auto &b : bulks) b = Bulk{};
  bulks[1].buf = mapBuffer; bulks[1].cap = sizeof(mapBuffer);
  bulks[2].buf = routeBuffer; bulks[2].cap = sizeof(routeBuffer);
  bulks[3].buf = rasterBuffer; bulks[3].cap = sizeof(rasterBuffer);
  mapShow = showBuffer;
  pendingAck = ""; navDirty = false; parsedMaps = parsedRoutes = 0; acceptRaster = true;
}

int main() {
  // Out of order, then completion: ACK only once all bytes are present.
  reset();
  receive(packet(2, 1, 2, 1, 6, 3, {4, 5, 6}));
  assert(!bulks[2].done && pendingAck.value.empty());
  receive(packet(2, 1, 2, 0, 6, 0, {1, 2, 3}));
  assert(bulks[2].done && parsedRoutes == 1 && pendingAck.value == "K,2,1");
  assert(std::equal(routeBuffer, routeBuffer + 6, std::vector<uint8_t>{1,2,3,4,5,6}.begin()));

  // Duplicate seq is not counted twice.
  reset();
  auto first = packet(2, 2, 2, 0, 6, 0, {1, 2, 3});
  receive(first); receive(first);
  assert(bulks[2].got == 1 && bulks[2].receivedBytes == 3);
  receive(packet(2, 2, 2, 1, 6, 3, {4, 5, 6}));
  assert(parsedRoutes == 1);

  // Reject zero-length, overflow and truncated logical coverage.
  reset();
  receive(packet(2, 3, 2, 0, 6, 0, {}));
  receive(packet(2, 3, 2, 1, 6, 5, {9, 9}));
  assert(bulks[2].got == 0);
  receive(packet(2, 3, 2, 0, 6, 0, {1, 2}));
  receive(packet(2, 3, 2, 1, 6, 3, {4, 5}));
  assert(!bulks[2].done && parsedRoutes == 0 && pendingAck.value.empty());
  assert(bulks[2].got == 0); // Must be eligible for a clean retry.

  // Decoder failure must not ACK, and permits the same transfer id to retry.
  reset(); acceptRaster = false;
  receive(packet(3, 4, 1, 0, 3, 0, {1, 2, 3}));
  assert(!bulks[3].done && pendingAck.value.empty() && bulks[3].receivedBytes == 0);
  acceptRaster = true;
  receive(packet(3, 4, 1, 0, 3, 0, {1, 2, 3}));
  assert(bulks[3].done && pendingAck.value == "K,3,4");
  return 0;
}
