#pragma once
// Shared projection from the map's local metric coordinate plane to its
// 466x466 logical viewport. No Arduino, ESP32, LVGL or allocation dependency.
// Device Canvas renderer and future LVGL vector map use the same math.
static inline void motonavProjectDelta(
    float dx, float dy, float scale, float cosHeading, float sinHeading,
    float centerX, float centerY, float &outX, float &outY) {
  outX = centerX + scale * (dx * cosHeading - dy * sinHeading);
  outY = centerY - scale * (dx * sinHeading + dy * cosHeading);
}
static inline void motonavProjectWorld(
    float x, float y, float camX, float camY,
    float scale, float cosHeading, float sinHeading,
    float centerX, float centerY, float &outX, float &outY) {
  motonavProjectDelta(x - camX, y - camY, scale, cosHeading, sinHeading,
                      centerX, centerY, outX, outY);
}
