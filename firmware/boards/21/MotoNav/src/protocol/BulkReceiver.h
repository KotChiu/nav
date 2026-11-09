#pragma once
// 地图和路线大块数据分包接收。由 MotoNav.ino 按依赖顺序包含，仅编译一次。
// 'M' 类型 编号 总片数(2) 片序号(2) 总字节(4) 偏移(4) + 数据
void handleBulk(const uint8_t *d, int n) {
  if (n < 15) return;
  int type = d[1], id = d[2];
  int parts = d[3] | (d[4] << 8), seq = d[5] | (d[6] << 8);
  uint32_t total = (uint32_t)rd32(d + 7), off = (uint32_t)rd32(d + 11);
  if (type < 1 || type > 7) return;
  Bulk &b = bulks[type];
  if (type == 7) {
    if (total < PlaceLabels::HeaderBytes || total > PlaceLabels::Capacity) return;
    const bool newPacket = id != b.id || total != b.total || parts != b.parts;
    if (newPacket && (seq != 0 || off != 0)) return;
    const size_t stride = newPacket ? n - 15 : placeLabelStride;
    if (!PlaceLabels::fragment(total, parts, seq, off, n - 15, stride)) return;
    placeLabelStride = stride;
    if (!b.buf) {
      b.cap = PlaceLabels::Capacity;
      b.buf = (uint8_t *)ps_malloc(b.cap);
    }
  }
  if (type == 6) {
    if (inNav) { pendingAck="PHOTO,BUSY"; return; }
    bool first = seq == 0 && off == 0;
    if (id == photoCancelledId) return;
    if (total != PhotoProtocol::Bytes) return;
    if (b.buf && PhotoProtocol::obsolete(b.id,id)) return;
    if (!b.buf || b.id != id) {
      if (!first) return; // A new image starts only at its first fragment.
      photoChunkStride = n - 15;
      photoCancelledId = -1;
    }
    if (!PhotoProtocol::chunk(total,parts,seq,off,n-15,photoChunkStride)) return;
    if (!b.buf) { b.cap=PhotoProtocol::Bytes; b.buf=(uint8_t *)ps_malloc(b.cap); b.id=-1; }
    if (!b.buf) { pendingAck="PHOTO,ERROR"; return; }
  }
  if (!b.buf || total == 0 || total > b.cap || parts == 0 || parts > 4096 || seq >= parts) return;
  if (id != b.id || total != b.total || parts != b.parts) {     // 新的一份：从头开始收
    b.id = id; b.total = total; b.parts = parts; b.got = 0; b.done = false;
    memset(b.mask, 0, sizeof(b.mask));
  }
  if (b.done) { pendingAck = "K," + String(type) + "," + String(id); return; }   // 重复的：再确认一次
  uint32_t len = n - 15;
  if (off >= total) return;
  if (off + len > total) len = total - off;
  if (!(b.mask[seq >> 3] & (1 << (seq & 7)))) {
    memcpy(b.buf + off, d + 15, len);
    b.mask[seq >> 3] |= (1 << (seq & 7));
    b.got++;
  }
  if (b.got >= b.parts) {
    b.done = true;
    if (type == 1) {
      uint8_t *t = mapShow; mapShow = b.buf; b.buf = t;   // 交换：新图显示，旧缓冲留着收下一块
      parseMap(mapShow, total);
    } else if (type == 2) {
      parseRoute(b.buf, total);
    } else if (type == 4) {
      parseGhost(b.buf, total);                        // v4.4 原路线
    } else if (type == 6) {
      if (!parsePhoto(b.buf,total)) { pendingAck="PHOTO,ERROR"; b.done=false; b.got=0; memset(b.mask,0,sizeof(b.mask)); return; }
    } else if (type == 7) {
      if (!placeLabels.replace(b.buf, total)) { b.id = -1; return; }
    } else if (type == 5) {
      parseCover(b.buf, total);                        // v7.12.10-cover 音乐封面（失败也确认，不重发）
    } else {
      if (!parseRasterWire(b.buf,total)) {
        // 没有成功解码就不回“已确认”，并允许相同编号完整重发。
        b.done=false; b.got=0; memset(b.mask,0,sizeof(b.mask));
        return;
      }
    }
    pendingAck = "K," + String(type) + "," + String(id);
    navDirty = true;
  }
}
