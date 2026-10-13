#pragma once
// DS1302 三线时钟驱动（CE=GPIO4、SCLK=GPIO2、I/O=GPIO16，见硬件设计引脚表）
// 时序（手册）：CE 高期间传输；写位在 SCLK 上升沿采样；读位在 SCLK 下降沿后有效。
// I/O 为双向脚：输出时推挽，读前释放为输入。DS1302 供电必须 3.3V（见硬件设计）。
#include <stdint.h>

#include "rtc/datetime.h"

class Ds1302 {
 public:
  // GPIO 初始化（CE 低、SCLK 低、I/O 释放）
  void begin();

  // 读当前时间并校验（R-RTC-06）；校验失败返回 false 且不修改 out
  bool readTime(DateTime& out);

  // 写入时间（先解锁写保护、写完上锁，秒寄存器 CH 清零启振）
  bool writeTime(const DateTime& t);

 private:
  uint8_t readReg(uint8_t addr);
  void writeReg(uint8_t addr, uint8_t val);
  void writeByte(uint8_t b);
  uint8_t readByte();
  void writeBit(bool b);
  bool readBit();
  void ceHigh();
  void ceLow();
};
