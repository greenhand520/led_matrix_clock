#pragma once
// TM1640 显存映射（纯逻辑，可主机侧单测）：
// 帧缓冲平面行位图 → TM1640 GRID/SEG 显存布局
// 红列 c → ram[模块][c]，绿列 c → ram[模块][8+c]；bit k = SEG(k+1) = 行 k
#include <stdint.h>

#include "display/framebuffer.h"

inline void composeTm1640Ram(const FrameBuffer& fb,
                             uint8_t ram[FrameBuffer::kModules][16]) {
  using Plane = FrameBuffer::Plane;
  for (uint8_t m = 0; m < FrameBuffer::kModules; ++m) {
    for (uint8_t col = 0; col < 8; ++col) {
      uint8_t r = 0;
      uint8_t g = 0;
      for (uint8_t row = 0; row < 8; ++row) {
        const uint8_t bit = static_cast<uint8_t>(1 << col);
        if (fb.rowBits(Plane::RED, m, row) & bit) {
          r |= static_cast<uint8_t>(1 << row);
        }
        if (fb.rowBits(Plane::GREEN, m, row) & bit) {
          g |= static_cast<uint8_t>(1 << row);
        }
      }
      ram[m][col] = r;
      ram[m][8 + col] = g;
    }
  }
}
