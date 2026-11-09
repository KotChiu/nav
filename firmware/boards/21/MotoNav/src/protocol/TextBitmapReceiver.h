#pragma once
// 手机文字位图分包接收。由 MotoNav.ino 按依赖顺序包含，仅编译一次。
// 'B' slot id part parts wLo wHi h offLo offHi + 数据
void handleBitmap(const uint8_t *d, int n) {
  if (n < 10) return;
  int slot = d[1], id = d[2], part = d[3], parts = d[4];
  int w = d[5] | (d[6] << 8), h = d[7];
  int off = d[8] | (d[9] << 8);
  if (slot >= IMG_SLOTS) return;
  TextImg &im = imgs[slot];
  if (w == 0 || h == 0 || parts == 0) {          // 空图：清掉这一栏
    im.showW = 0; im.showH = 0; im.recvId = -1;
    ++textImageRevision[slot];
    navDirty = true;
    if ((slot >= 3 && slot <= 4) || (slot >= 7 && slot <= 11)) wxDirty = true;
    if (slot == 5 || slot == 6 || slot == 12) musicDirty = true;
    if (slot >= 13 && slot < 13 + STOCK_MAX) stockDirty = true;
    return;
  }
  if (slot == 12 && (!musicLyrics || !hidMusicMode || !appleMedia.available ||
      lyricImageGeneration != appleMedia.generation || lyricImageTitleRevision != appleMedia.values[0].revision || lyricImageArtistRevision != appleMedia.values[1].revision)) return;
  int total = ((w + 1) / 2) * h;
  // Reject invalid fragments before any write or bit shift. BLE messages are
  // untrusted; a malformed offset must never be turned into a negative memcpy
  // length. Keep the original wire format and in-order/any-order behavior.
  if (total <= 0 || total > IMG_MAX || parts > 32 || part >= parts ||
      part >= 32 || !im.recv || !im.show || off >= total ||
      n <= 10 || n - 10 > total - off) return;
  if (id != im.recvId) {                          // 新的一张图：重新开始接收
    im.recvId = id; im.recvW = w; im.recvH = h; im.recvParts = parts; im.recvMask = 0;
  }
  const int len = n - 10;
  memcpy(im.recv + off, d + 10, len);
  im.recvMask |= (1UL << part);
  uint32_t full = (parts >= 32) ? 0xFFFFFFFFUL : ((1UL << parts) - 1);
  if ((im.recvMask & full) == full) {             // 收齐了：换成显示用的图
    uint8_t *t = im.show; im.show = im.recv; im.recv = t;
    im.showW = im.recvW; im.showH = im.recvH;
    ++textImageRevision[slot];
    im.recvId = -1;
    navDirty = true;
    if ((slot >= 3 && slot <= 4) || (slot >= 7 && slot <= 11)) wxDirty = true;
    if (slot == 5 || slot == 6 || slot == 12) musicDirty = true;                // 音乐文字图片收齐后刷新
    if (slot >= 13 && slot < 13 + STOCK_MAX) stockDirty = true;
  }
}
