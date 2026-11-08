#pragma once
// 电量轮询及状态消息。由 MotoNav.ino 按依赖顺序包含，仅编译一次。
String pendingStat2 = "";
// v6.8 每 5 秒读一次；有变化就重画、通知手机
void batPoll() {
  if (!pmuOK) return;
  unsigned long now = millis();
  if (batAt && now - batAt < 5000) return;
  batAt = now;

  int adcMv = analogReadMilliVolts(BAT_ADC_PIN);
  int mv = (int)(adcMv * 3.0f / 0.992857f);
  if (mv < 3000) mv = 3000;
  if (mv > 4300) mv = 4300;
  int pct = constrain((mv - 3000) * 100 / 1200, 0, 100);
  bool present = adcMv > 500;
  bool chg = false;
  bool usb = false;

  if (!present) pct = -1;
  bool displayChanged = pct != batPct || chg != batChg || usb != usbIn;
  bool changed = pct != batPct || chg != batChg || usb != usbIn || mv != batMv;
  batPct = pct; batChg = chg; usbIn = usb; batMv = mv;
  if (mediaHid && changed && pct >= 0) mediaHid->setBatteryLevel((uint8_t)constrain(pct, 0, 100));
  if (displayChanged) {
    if (!lvglUiReady && onWaitScreen && page == 0 && !sliding) drawStatus();   // legacy canvas only
    else if (inNav) navDirty = true;
  }
  if (changed || now - batSentAt > 60000) {
    batSentAt = now;
    pendingBat = "V," + String(batPct) + "," + String(batChg ? 1 : 0) + "," + String(usbIn ? 1 : 0) + "," + String(batMv);
  }
}
