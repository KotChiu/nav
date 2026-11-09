#pragma once
#include <stdint.h>
#include <stddef.h>
namespace PhotoProtocol {
constexpr unsigned Pixels = 232;
constexpr size_t Bytes = 8 + Pixels * Pixels * 2;
inline bool obsolete(int current, unsigned incoming) { return current >= 0 && incoming != unsigned(current) && uint8_t(incoming-current) >= 128; }
inline bool valid(const uint8_t *p, size_t n) {
  return p && n == Bytes && p[0]=='P' && p[1]=='H' && p[2]=='O' && p[3]==1 &&
    (p[4] | (unsigned(p[5])<<8))==Pixels && (p[6] | (unsigned(p[7])<<8))==Pixels;
}
inline bool chunk(size_t total, unsigned parts, unsigned seq, size_t off, size_t len, size_t stride) {
  return total==Bytes && stride>0 && parts==(total+stride-1)/stride && seq<parts &&
    off==size_t(seq)*stride && off<total && len==(total-off<stride?total-off:stride);
}
}
