#pragma once
// Badge photo persistence lives in the existing FAT partition, if mounted.
// Never silently format the FAT partition: it can contain other user files.
#include <FFat.h>
#include "../../ui/BadgePacket.h"

// PageNavigation.h is included after this file in the sketch entrypoint.
void drawBadgePage();

static const char *const BADGE_PATH = "/badge.bin";
static const char *const BADGE_TEMP_PATH = "/badge.tmp";
static const char *const BADGE_BACKUP_PATH = "/badge.bak";

// The backup holds the previous confirmed image across interrupted updates.
static bool badgeReadFile(const char *path) {
  if (!badgePixels) return false;
  File file = FFat.open(path, FILE_READ);
  if (!file) return false;
  if (file.size() != MotoNavBadge::PACKET_BYTES) { file.close(); return false; }
  uint8_t header[MotoNavBadge::HEADER_BYTES] = {};
  if (file.read(header, sizeof(header)) != sizeof(header)) { file.close(); return false; }
  if (header[0] != 'B' || header[1] != 'D' || header[2] != 'G' ||
      header[3] != '1' ||
      MotoNavBadge::le16(header + 4) != MotoNavBadge::SIZE ||
      MotoNavBadge::le16(header + 6) != MotoNavBadge::SIZE) {
    file.close(); return false;
  }
  const size_t count = file.readBytes(reinterpret_cast<char *>(badgePixels),
                                      MotoNavBadge::PIXEL_BYTES);
  file.close();
  return count == MotoNavBadge::PIXEL_BYTES &&
         MotoNavBadge::le32(header + 8) ==
         MotoNavBadge::crc32(reinterpret_cast<const uint8_t *>(badgePixels),
                             MotoNavBadge::PIXEL_BYTES);
}

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
  // An interrupted rename leaves the earlier valid image in .bak. Never
  // format or delete either copy during startup recovery.
  if (badgeReadFile(BADGE_PATH) || badgeReadFile(BADGE_BACKUP_PATH)) {
    badgeActive = true;
    badgeStored = true;
    ++badgeRevision;
    Serial.println("BADGE: recovered validated persisted photo");
  } else if (FFat.exists(BADGE_PATH) || FFat.exists(BADGE_BACKUP_PATH)) {
    Serial.println("BADGE: stored images failed CRC/size validation");
  }
}

static bool badgePersist(const uint8_t *packet, size_t bytes) {
  if (!badgeStorageReady) return false;
  // FFat FILE_WRITE appends on some implementations; explicitly discard an
  // old/incomplete temporary file before creating the new one.
  if (FFat.exists(BADGE_TEMP_PATH) && !FFat.remove(BADGE_TEMP_PATH))
    return false;
  File file = FFat.open(BADGE_TEMP_PATH, FILE_WRITE);
  if (!file) return false;
  const bool wrote = file.write(packet, bytes) == bytes;
  file.flush();
  file.close();
  if (!wrote) { FFat.remove(BADGE_TEMP_PATH); return false; }

  // Validate the on-disk copy's exact size before touching the old photo.
  File check = FFat.open(BADGE_TEMP_PATH, FILE_READ);
  if (!check) { FFat.remove(BADGE_TEMP_PATH); return false; }
  const bool complete = check.size() == bytes;
  check.close();
  if (!complete) { FFat.remove(BADGE_TEMP_PATH); return false; }

  const bool hadOld = FFat.exists(BADGE_PATH);
  if (hadOld) {
    // Active photo remains available until a complete new file exists.
    // Previous .bak is redundant while .bin exists.
    if (FFat.exists(BADGE_BACKUP_PATH) && !FFat.remove(BADGE_BACKUP_PATH))
      return false;
    if (!FFat.rename(BADGE_PATH, BADGE_BACKUP_PATH)) return false;
  }
  if (!FFat.rename(BADGE_TEMP_PATH, BADGE_PATH)) {
    // New image failed to commit: restore the old image, if possible. The
    // backup remains recoverable at startup even if this rename fails.
    if (hadOld) FFat.rename(BADGE_BACKUP_PATH, BADGE_PATH);
    return false;
  }
  // Keep the previous image as a one-generation fallback in case the new
  // FAT file becomes corrupted after a power loss.
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
  // Include transfer ID to prevent an old notification from being mistaken
  // for a later photo upload.
  pendingBadgeStatus = String("BDG,SAVED,") + bulks[MotoNavBadge::BULK_TYPE].id +
                       (badgeStored ? ",1" : ",0");
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
