#pragma once
// 时间结构与纯逻辑（无硬件依赖，可主机侧单测）
#include <stdint.h>

struct DateTime {
  uint16_t year;  // 2000..2099（DS1302 两位年 + 2000）
  uint8_t mon;    // 1..12
  uint8_t day;    // 1..31
  uint8_t hour;   // 0..23
  uint8_t min;    // 0..59
  uint8_t sec;    // 0..59
  uint8_t wday;   // 1..7（DS1302 星期寄存器）
};

// 时间合法性校验（需求 R-RTC-06）：超界视为无效，调用方保持上次有效值
inline bool validDateTime(const DateTime& t) {
  return t.year >= 2000 && t.year <= 2099 && t.mon >= 1 && t.mon <= 12 &&
         t.day >= 1 && t.day <= 31 && t.hour <= 23 && t.min <= 59 &&
         t.sec <= 59 && t.wday >= 1 && t.wday <= 7;
}

// 24 小时制 → 12 小时制：返回 1..12；pm 为真表示下午
inline uint8_t to12h(uint8_t hour24, bool* pm) {
  const bool isPm = hour24 >= 12;
  if (pm != nullptr) {
    *pm = isPm;
  }
  uint8_t h = hour24 % 12;
  return (h == 0) ? 12 : h;
}
