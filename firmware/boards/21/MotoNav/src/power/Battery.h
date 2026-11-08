#pragma once
// Battery monitor for the 2.1-inch board. It uses the GPIO4 battery ADC
// instead of the AXP2101 fuel gauge found on the original 1.75-inch board.
#define PMU_ADDR 0x34
bool pmuOK = false;
int batPct = -1;                       // -1 = 没接电池
bool batChg = false, usbIn = false;
int batMv = 0;
unsigned long batAt = 0, batSentAt = 0;
String pendingBat = "";

int pmuRead(uint8_t reg) {
  (void)reg;
  return -1;
}
void pmuSetBit(uint8_t reg, uint8_t bit) {
  (void)reg;
  (void)bit;
}

void batInit() {
  analogReadResolution(12);
  pmuOK = true;
}

// Charging/USB state is not exposed by a register on this board. Treat it as
// external power so the idle shutdown policy remains conservative.
void idlePowerPollSupply(bool force = false) {
  const uint32_t now = millis();
  if (!force && idlePowerSupply.sampled &&
      (uint32_t)(now - idlePowerSupply.checkedAt) < 500) return;
  const bool wasBattery = idlePowerSupplyAllowsOff(&idlePowerSupply, now);
  const int status = -1;
  idlePowerSupplyUpdate(&idlePowerSupply, now, status);
  // Unplugging / recovering a valid battery-only reading starts a full interval.
  if (!wasBattery && idlePowerSupplyAllowsOff(&idlePowerSupply, now))
    idlePowerOffReset(&idlePowerOffTimer, now);
}

// 小电池图标：x0,y0 = 左上角（电池身 24x12，右边小凸头，再右边是百分比数字）
