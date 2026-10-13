#pragma once
// 舒适度着色规则（需求 R-ENV-05，边界由需求基线定义）
// 温度单位 0.1℃；湿度 %RH
//   绿（舒适）    ：18.0–26.0℃ / 40–70%
//   橙（轻微不适）：12.0–17.9、26.1–30.0℃ / 30–39、71–80%
//   红（严重不适）：其余
#include <stdint.h>

#include "display/framebuffer.h"

inline Color comfortColorTemp(int16_t tenths) {
  if (tenths >= 180 && tenths <= 260) {
    return Color::GREEN;
  }
  if (tenths >= 120 && tenths <= 300) {
    return Color::ORANGE;
  }
  return Color::RED;
}

inline Color comfortColorHum(uint8_t percent) {
  if (percent >= 40 && percent <= 70) {
    return Color::GREEN;
  }
  if (percent >= 30 && percent <= 80) {
    return Color::ORANGE;
  }
  return Color::RED;
}
