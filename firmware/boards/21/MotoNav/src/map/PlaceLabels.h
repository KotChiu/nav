#pragma once
#include <stddef.h>
#include <stdint.h>

// 'P', version, frame LE16, count; then x/y metres LE32, width, height and glyph rows.
// Version 1 uses 4-bit coverage; version 2 uses 2-bit coverage for larger glyphs.
struct PlaceLabels {
  static constexpr size_t HeaderBytes = 5, EntryBytes = 10;
  static constexpr uint8_t MaxCount = 8, MaxWidth = 186, MaxHeight = 44;
  static constexpr size_t GlyphBytes = ((MaxWidth + 3) / 4) * MaxHeight;
  static constexpr size_t Capacity = HeaderBytes + MaxCount * (EntryBytes + GlyphBytes);

  struct Label {
    int32_t x, y;
    uint8_t width, height;
    const uint8_t *pixels;
  };

  uint16_t frame = 0;
  uint8_t count = 0;
  uint8_t version = 1;
  uint8_t *storage = nullptr;
  Label labels[MaxCount];

  void clear() { count = 0; }

  static bool fragment(size_t total, unsigned parts, unsigned seq, size_t off, size_t len, size_t stride) {
    return stride && parts == (total + stride - 1) / stride && seq < parts &&
      off == seq * stride && off < total && len == (total - off < stride ? total - off : stride);
  }

  static int32_t coordinate(const uint8_t *p) {
    uint32_t value = uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
    return static_cast<int32_t>(value);
  }

  static unsigned stride(unsigned width, uint8_t format) {
    return format == 2 ? (width + 3) / 4 : (width + 1) / 2;
  }

  static uint8_t coverage(const uint8_t *row, int x, uint8_t format) {
    if (format == 2) return ((row[x / 4] >> (6 - 2 * (x % 4))) & 3) * 5;
    return x & 1 ? row[x / 2] & 15 : row[x / 2] >> 4;
  }

  bool replace(uint8_t *&buffer, size_t bytes) {
    const uint8_t *p = buffer;
    if (!p || bytes < HeaderBytes || p[0] != 'P' || (p[1] != 1 && p[1] != 2) || p[4] > MaxCount) return false;
    const uint8_t format = p[1];
    const unsigned maxWidth = format == 2 ? MaxWidth : 108;
    const unsigned maxHeight = format == 2 ? MaxHeight : 24;
    size_t offset = HeaderBytes;
    // Validate the entire packet before replacing the displayed labels.
    for (int i = 0; i < p[4]; i++) {
      if (bytes - offset < EntryBytes) return false;
      const unsigned w = p[offset + 8], h = p[offset + 9];
      const size_t size = stride(w, format) * h;
      if (!w || !h || w > maxWidth || h > maxHeight || size > bytes - offset - EntryBytes) return false;
      offset += EntryBytes + size;
    }
    if (offset != bytes) return false;
    uint8_t *previous = storage;
    storage = buffer;
    buffer = previous;
    frame = p[2] | (uint16_t(p[3]) << 8);
    count = p[4];
    version = format;
    p += HeaderBytes;
    for (int i = 0; i < count; i++) {
      Label &label = labels[i];
      label.x = coordinate(p);
      label.y = coordinate(p + 4);
      label.width = p[8];
      label.height = p[9];
      const size_t size = stride(label.width, format) * label.height;
      label.pixels = p + EntryBytes;
      p += EntryBytes + size;
    }
    return true;
  }
};
