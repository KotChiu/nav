#pragma once
#include "../../PhotoProtocol.h"
uint16_t *photoPixels = nullptr;
bool photoValid = false;
bool photoDirty = false;
size_t photoChunkStride = 0;
int photoCancelledId = -1;
bool parsePhoto(const uint8_t *p, size_t n) {
  if (!PhotoProtocol::valid(p,n)) return false;
  if (!photoPixels) photoPixels=(uint16_t *)ps_malloc(PhotoProtocol::Pixels*PhotoProtocol::Pixels*2);
  if (!photoPixels) return false;
  memcpy(photoPixels,p+8,n-8);
  photoValid=true; photoDirty=true;
  return true;
}
