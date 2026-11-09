#pragma once
// Badge photo persistence lives in the existing FAT partition, if mounted.
// Never silently format the FAT partition: it can contain other user files.
#include <FFat.h>
#include "../../ui/BadgePacket.h"

// PageNavigation.h is included after this file in the sketch entrypoint.
void drawBadgePage();

static const char *const BADGE_PATH = "/badge.bin";
static const char *const BADGE_TEMP_PATH = "/badge.tmp";

void badgeInit() {
  badgePixels = static_cast<uint16_t *>(ps_malloc(MotoNavBadge::PIXEL_BYTES));
  if (!badgePixels) {
    Serial.println("BADGE: PSRAM allocation failed");
    return;
  }
  badgeStorageReady = FFat.begin(false);
  if (!badgeStorageReady) {
    Serial.println("BADGE: FFat not mounted (no auto-format); RAM-only mode");
    return;
  }
  File file = FFat.open(BADGE_PATH, FILE_READ);
  if (!file) return;
  if (file.size() != MotoNavBadge::PACKET_BYTES) { file.close(); return; }
  uint8_t header[MotoNavBadge::HEADER_BYTES] = {};
  if (file.read(header, sizeof(header)) != sizeof(header)) { file.close(); return; }
  // Check the header BEFORE reading a potentially invalid image.
  if (header[0] != 'B' || header[1] != 'D' || header[2] != 'G' || header[3] != '1' ||
      MotoNavBadge::le16(header + 4) != MotoNavBadge::SIZE ||
      MotoNavBadge::le16(header + 6) != MotoNavBadge::SIZE) {
    file.close(); return;
  }
  const size_t count = file.readBytes(reinterpret_cast<char *>(badgePixels),
                                      MotoNavBadge::PIXEL_BYTES);
  file.close();
  if (count != MotoNavBadge::PIXEL_BYTES ||
      MotoNavBadge::le32(header + 8) !=
      MotoNavBadge::crc32(reinterpret_cast<const uint8_t *>(badgePixels),
                          MotoNavBadge::PIXEL_BYTES)) {
    Serial.println("BADGE: persisted image CRC failed");
    return;
  }
  badgeActive = true;
  badgeStored = true;
  ++badgeRevision;
  Serial.println("BADGE: loaded persisted photo");
}

static bool badgePersist(const uint8_t *packet, size_t bytes) {
  if (!badgeStorageReady) return false;
  File file = FFat.open(BADGE_TEMP_PATH, FILE_WRITE);
  if (!file) return false;
  const bool wrote = file.write(packet, bytes) == bytes;
  file.flush();
  file.close();
  if (!wrote) { FFat.remove(BADGE_TEMP_PATH); return false; }
  if (!FFat.rename(BADGE_TEMP_PATH, BADGE_PATH)) {
    // FAT drivers do not all support overwrite-by-rename.
    if (FFat.exists(BADGE_PATH)) FFat.remove(BADGE_PATH);
    if (!FFat.rename(BADGE_TEMP_PATH, BADGE_PATH)) {
      FFat.remove(BADGE_TEMP_PATH);
      return false;
    }
  }
  return true;
}

// Called only from the BLE main-loop dispatch, never from BLE GATT callback.
bool parseBadge(const uint8_t *packet, uint32_t len) {
  if (!MotoNavBadge::valid(packet, len) || !badgePixels) return false;
  memcpy(badgePixels, packet + MotoNavBadge::HEADER_BYTES,
         MotoNavBadge::PIXEL_BYTES);
  badgeActive = true;
  ++badgeRevision;
  badgeStored = badgePersist(packet, len);
  pendingBadgeStatus = badgeStored ? "BDG,SAVED,1" : "BDG,SAVED,0";
  // A new photo appears immediately when idle, without a swipe through
  // navigation pages. Never replace the screen during active navigation,
  // a call, pairing or an animation.
  if (!inNav && !sliding && !pairOn && !callOn && !fxOn) {
    page = PAGE_BADGE;
    pageAt = millis();
    drawBadgePage();
  }
  if (page == PAGE_BADGE) {
    forceFullFlush = true;
    needFlush = true;
  }
  Serial.printf("BADGE: accepted %u bytes, persisted=%d\n",
                (unsigned)len, (int)badgeStored);
  return true;
}
