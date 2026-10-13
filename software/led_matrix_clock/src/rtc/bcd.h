#pragma once
// BCD 编解码（DS1302 寄存器为 BCD 格式；纯逻辑，可主机侧单测）
#include <stdint.h>

// 十进制 0..99 → BCD
inline uint8_t dec2bcd(uint8_t v) {
  return static_cast<uint8_t>(((v / 10) << 4) | (v % 10));
}

// BCD → 十进制
inline uint8_t bcd2dec(uint8_t v) {
  return static_cast<uint8_t>(((v >> 4) * 10) + (v & 0x0F));
}
