// DS1302 驱动实现（GPIO 位带）
#include "rtc/ds1302.h"

#include <Arduino.h>

#include "rtc/bcd.h"

namespace {
constexpr uint8_t kPinCe = 4;
constexpr uint8_t kPinSclk = 2;
constexpr uint8_t kPinIo = 16;

constexpr uint8_t kRegSec = 0x00;
constexpr uint8_t kRegMin = 0x01;
constexpr uint8_t kRegHour = 0x02;
constexpr uint8_t kRegWday = 0x03;
constexpr uint8_t kRegDay = 0x04;
constexpr uint8_t kRegMon = 0x05;
constexpr uint8_t kRegYear = 0x06;
constexpr uint8_t kRegWp = 0x07;  // 写保护

constexpr uint8_t kCmdRead = 0x01;
constexpr uint8_t kSecChMask = 0x7F;   // 秒寄存器 bit7 = CH（停振）
constexpr uint8_t kHour12Mask = 0x80;  // 小时寄存器 bit7 = 12/24 标志
constexpr uint8_t kHour12ValMask = 0x1F;
constexpr uint8_t kHour12PmMask = 0x20;
constexpr uint8_t kWpLock = 0x80;

constexpr int kHalfUs = 2;  // 位周期半程
}  // namespace

void Ds1302::begin() {
  pinMode(kPinCe, OUTPUT);
  pinMode(kPinSclk, OUTPUT);
  digitalWrite(kPinCe, LOW);
  digitalWrite(kPinSclk, LOW);
  pinMode(kPinIo, INPUT);  // 空闲释放总线
}

void Ds1302::ceHigh() {
  digitalWrite(kPinCe, HIGH);
  delayMicroseconds(4);  // CE 到首 SCLK 建立时间
}

void Ds1302::ceLow() {
  digitalWrite(kPinCe, LOW);
  delayMicroseconds(4);
}

void Ds1302::writeBit(bool b) {
  digitalWrite(kPinSclk, LOW);
  pinMode(kPinIo, OUTPUT);
  digitalWrite(kPinIo, b ? HIGH : LOW);
  delayMicroseconds(kHalfUs);
  digitalWrite(kPinSclk, HIGH);  // 上升沿采样
  delayMicroseconds(kHalfUs);
  digitalWrite(kPinSclk, LOW);
}

bool Ds1302::readBit() {
  pinMode(kPinIo, INPUT);  // 释放，DS1302 驱动
  delayMicroseconds(kHalfUs);
  const bool b = digitalRead(kPinIo) != 0;  // 下降沿后采样
  digitalWrite(kPinSclk, HIGH);
  delayMicroseconds(kHalfUs);
  digitalWrite(kPinSclk, LOW);  // 下降沿 → 输出下一位
  return b;
}

void Ds1302::writeByte(uint8_t b) {
  for (uint8_t i = 0; i < 8; ++i) {
    writeBit((b >> i) & 1);  // LSB 先行
  }
}

uint8_t Ds1302::readByte() {
  uint8_t v = 0;
  for (uint8_t i = 0; i < 8; ++i) {
    if (readBit()) {
      v |= static_cast<uint8_t>(1 << i);  // LSB 先行
    }
  }
  return v;
}

void Ds1302::writeReg(uint8_t addr, uint8_t val) {
  ceHigh();
  writeByte(static_cast<uint8_t>(0x80 | (addr << 1)));  // 写命令（bit0=0）
  writeByte(val);
  ceLow();
  pinMode(kPinIo, INPUT);
}

uint8_t Ds1302::readReg(uint8_t addr) {
  ceHigh();
  writeByte(static_cast<uint8_t>(0x80 | (addr << 1) | kCmdRead));
  const uint8_t v = readByte();
  ceLow();
  pinMode(kPinIo, INPUT);
  return v;
}

bool Ds1302::readTime(DateTime& out) {
  DateTime t;
  t.sec = static_cast<uint8_t>(bcd2dec(readReg(kRegSec) & kSecChMask));
  t.min = bcd2dec(readReg(kRegMin) & 0x7F);
  const uint8_t hr = readReg(kRegHour);
  if (hr & kHour12Mask) {
    // 12 小时制寄存器（bit5=PM）→ 转 24 小时制
    const uint8_t h12 = static_cast<uint8_t>(bcd2dec(hr & kHour12ValMask) % 12);
    t.hour = static_cast<uint8_t>(h12 + ((hr & kHour12PmMask) ? 12 : 0));
  } else {
    t.hour = bcd2dec(hr & 0x3F);
  }
  t.wday = readReg(kRegWday) & 0x07;
  t.day = bcd2dec(readReg(kRegDay) & 0x3F);
  t.mon = bcd2dec(readReg(kRegMon) & 0x1F);
  t.year = static_cast<uint16_t>(2000 + bcd2dec(readReg(kRegYear)));
  if (!validDateTime(t)) {
    return false;
  }
  out = t;
  return true;
}

bool Ds1302::writeTime(const DateTime& t) {
  if (!validDateTime(t)) {
    return false;
  }
  writeReg(kRegWp, 0x00);  // 解锁写保护
  writeReg(kRegSec, dec2bcd(t.sec));  // CH=0 启振
  writeReg(kRegMin, dec2bcd(t.min));
  writeReg(kRegHour, dec2bcd(t.hour));  // 24 小时制（bit7=0）
  writeReg(kRegWday, t.wday);
  writeReg(kRegDay, dec2bcd(t.day));
  writeReg(kRegMon, dec2bcd(t.mon));
  writeReg(kRegYear, dec2bcd(static_cast<uint8_t>(t.year - 2000)));
  writeReg(kRegWp, kWpLock);  // 上锁
  return true;
}
