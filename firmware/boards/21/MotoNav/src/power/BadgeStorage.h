#pragma once
// Prefer the TF card. Keep support for the existing internal FAT storage.
#include <FFat.h>
#include <SD_MMC.h>
#include "../../ui/BadgePacket.h"

// PageNavigation.h is included after this file in the sketch entrypoint.
void drawBadgePage();

struct BadgePaths {
  const char *primary;
  const char *temporary;
  const char *backup;
};
static constexpr BadgePaths internalBadgePaths = {"/badge.bin", "/badge.tmp", "/badge.bak"};
static constexpr BadgePaths sdBadgePaths = {"/motonav/badge.bin", "/motonav/badge.tmp", "/motonav/badge.bak"};
static bool badgeRecoveredFromBackup = false;
static fs::FS *badgeStore = nullptr;

static const BadgePaths &badgePaths(fs::FS &store) {
  return &store == &SD_MMC ? sdBadgePaths : internalBadgePaths;
}

// The backup holds the previous confirmed image across interrupted updates.
static bool badgeReadFile(fs::FS &store, const char *path) {
  if (!badgePixels) return false;
  File file = store.open(path, FILE_READ);
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

static bool badgeLoad(fs::FS &store) {
  const BadgePaths &paths = badgePaths(store);
  const bool primaryGood = badgeReadFile(store, paths.primary);
  const bool backupGood = !primaryGood && badgeReadFile(store, paths.backup);
  if (!primaryGood && !backupGood) {
    if (store.exists(paths.primary) || store.exists(paths.backup))
      Serial.println("BADGE: stored images failed CRC/size validation");
    return false;
  }
  if (&store == badgeStore) badgeRecoveredFromBackup = backupGood;
  badgeActive = true;
  badgeStored = true;
  ++badgeRevision;
  Serial.println("BADGE: recovered validated persisted photo");
  return true;
}

const char *badgeStorageLabel() {
  if (badgeStore == &SD_MMC) return "SD CARD READY";
  if (badgeStore == &FFat) return "INTERNAL STORAGE";
  return "TEMPORARY DISPLAY";
}

void badgeInit() {
  badgePixels = static_cast<uint16_t *>(ps_malloc(MotoNavBadge::PIXEL_BYTES));
  if (!badgePixels) {
    Serial.println("BADGE: PSRAM allocation failed");
    return;
  }
  // DAT3/CS stays high to select native SD mode; begin never formats media.
  if (SD_MMC.setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_DATA) &&
      SD_MMC.begin("/sdcard", true, false)) {
    badgeStore = &SD_MMC;
    Serial.printf("BADGE: SD mounted, capacity=%llu MiB\n",
                  (unsigned long long)(SD_MMC.cardSize() / (1024ULL * 1024)));
    if (badgeLoad(SD_MMC)) return;
  } else {
    Serial.println("BADGE: SD unavailable");
  }
  // Preserve access to photos saved by older firmware when an empty SD is inserted.
  if (FFat.begin(false)) {
    if (!badgeStore) badgeStore = &FFat;
    badgeLoad(FFat);
  }
  Serial.printf("BADGE: %s\n", badgeStorageLabel());
}

static bool badgePersist(const uint8_t *packet, size_t bytes) {
  if (!badgeStore) return false;
  fs::FS &store = *badgeStore;
  const BadgePaths &paths = badgePaths(store);
  if (badgeStore == &SD_MMC && !store.exists("/motonav") && !store.mkdir("/motonav"))
    return false;
  // FAT FILE_WRITE appends on some implementations; explicitly discard an
  // old/incomplete temporary file before creating the new one.
  if (store.exists(paths.temporary) && !store.remove(paths.temporary))
    return false;
  File file = store.open(paths.temporary, FILE_WRITE);
  if (!file) return false;
  const bool wrote = file.write(packet, bytes) == bytes;
  file.flush();
  file.close();
  if (!wrote) { store.remove(paths.temporary); return false; }

  // Validate the on-disk copy's exact size before touching the old photo.
  File check = store.open(paths.temporary, FILE_READ);
  if (!check) { store.remove(paths.temporary); return false; }
  const bool complete = check.size() == bytes;
  check.close();
  if (!complete) { store.remove(paths.temporary); return false; }

  const bool hadOld = store.exists(paths.primary);
  if (hadOld && badgeRecoveredFromBackup) {
    // Current .bin is damaged and the last valid image is in .bak. Never
    // destroy the only valid backup to make room for an invalid primary.
    if (!store.remove(paths.primary)) return false;
  } else if (hadOld) {
    // A current valid .bin exists; rotating an older backup is safe.
    if (store.exists(paths.backup) && !store.remove(paths.backup))
      return false;
    if (!store.rename(paths.primary, paths.backup)) return false;
  }
  if (!store.rename(paths.temporary, paths.primary)) {
    // If prior .bin was healthy, restore from the backup. For a recovered
    // boot the valid .bak remains available for the next startup.
    if (hadOld && !badgeRecoveredFromBackup)
      store.rename(paths.backup, paths.primary);
    return false;
  }
  badgeRecoveredFromBackup = false;
  // Keep one previous generation as fallback after an interrupted update.
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
