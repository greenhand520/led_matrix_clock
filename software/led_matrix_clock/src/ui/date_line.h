#pragma once
// 日期行文本拼接（纯逻辑，可主机侧单测；布局稿 §4）
// date_order：0 = YYYY-MM-DD WWT（默认）；1 = DD-MM-YYYY WWT；2 = MM-DD-YYYY WWT
// 星期约定：wday 1=周日 .. 7=周六（DS1302 星期寄存器约定，P6 对时时写入）
#include <stdint.h>
#include <stdio.h>

#include "rtc/datetime.h"

inline void buildDateText(const DateTime& t, uint8_t order, char* out, unsigned n) {
  static const char* kDays[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
  const char* wd = (t.wday >= 1 && t.wday <= 7) ? kDays[t.wday - 1] : "???";
  switch (order) {
    case 1:
      snprintf(out, n, "%02u-%02u-%04u %s", t.day, t.mon, t.year, wd);
      break;
    case 2:
      snprintf(out, n, "%02u-%02u-%04u %s", t.mon, t.day, t.year, wd);
      break;
    default:
      snprintf(out, n, "%04u-%02u-%02u %s", t.year, t.mon, t.day, wd);
      break;
  }
}
