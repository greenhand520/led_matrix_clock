#pragma once
// 自动亮度（纯逻辑，可主机侧单测）：5516 光敏分压 → A0，8 档映射（R-BRI-01/02）
// 环境光越亮 → 显示越暗（方向常量 kLightHighMeansBright，随硬件分压方向调整）
// 平滑：滑动平均 + 连续 2 次同目标才换档（迟滞，防闪烁）
#include <stdint.h>

class AutoBrightness {
 public:
  // 输入 ADC 值 0..1023，返回亮度档 0..7
  uint8_t update(uint16_t adc) {
    // 滑动平均 avg = (3*avg + adc)/4
    if (!primed_) {
      avg_ = adc;
      primed_ = true;
    } else {
      avg_ = static_cast<uint16_t>((avg_ * 3u + adc) / 4u);
    }

    // 目标档位：暗 → 高档（亮屏），亮 → 低档
    uint8_t target = static_cast<uint8_t>(7u - (avg_ >> 7));  // 0..1023 → 7..0
    if (target > 7) {
      target = 7;
    }

    // 迟滞：连续两次同目标才切换
    if (target != pending_) {
      pending_ = target;
      sameCount_ = 0;
    } else if (target != level_) {
      if (++sameCount_ >= 2) {
        level_ = target;
        sameCount_ = 0;
      }
    }
    return level_;
  }

  uint8_t level() const { return level_; }

 private:
  uint16_t avg_ = 512;
  uint8_t level_ = 5;
  uint8_t pending_ = 5;
  uint8_t sameCount_ = 0;
  bool primed_ = false;
};
