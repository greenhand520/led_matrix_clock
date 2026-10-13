#pragma once
// 数字垂直滚动动画（纯逻辑，可主机侧单测；布局稿 §2）：
// 数字位变化时 8 步 × 50ms（≈400ms）：新字形从上滚入、旧字形向下移出
#include <stdint.h>

#include "display/framebuffer.h"

// 槽位静态配置：坐标/字号/颜色（布局稿 §2 时间主线）
struct DigitSlot {
  int16_t x;
  int16_t y;
  FontKind font;
  Color color;
};

class DigitAnimator {
 public:
  static constexpr int kSlots = 6;
  static constexpr int kSteps = 8;  // 8 步 × 50ms

  // 提交槽位目标字符；与当前不同则启动滚动（首帧直接落位）
  void feed(uint8_t idx, char target) {
    if (idx >= kSlots) {
      return;
    }
    if (!inited_[idx]) {
      cur_[idx] = target;
      inited_[idx] = true;
      return;
    }
    if (target != cur_[idx]) {
      prev_[idx] = cur_[idx];
      cur_[idx] = target;
      p_[idx] = 0;
      active_[idx] = true;
    }
  }

  // 每 50ms 步进一次
  void step() {
    for (int i = 0; i < kSlots; ++i) {
      if (active_[i]) {
        if (++p_[i] >= kSteps) {
          p_[i] = 0;
          active_[i] = false;
        }
      }
    }
  }

  // 绘制：滚动槽合成新旧字形（新字形 y+p-8 从上滚入，旧字形 y+p 向下移出）
  void draw(FrameBuffer& fb, const DigitSlot slots[kSlots]) const {
    for (int i = 0; i < kSlots; ++i) {
      if (!inited_[i]) {
        continue;
      }
      const DigitSlot& s = slots[i];
      if (active_[i]) {
        fb.drawChar(prev_[i], s.x, s.y + p_[i], s.color, s.font);
        fb.drawChar(cur_[i], s.x, s.y + p_[i] - kSteps, s.color, s.font);
      } else {
        fb.drawChar(cur_[i], s.x, s.y, s.color, s.font);
      }
    }
  }

  bool animating() const {
    for (int i = 0; i < kSlots; ++i) {
      if (active_[i]) {
        return true;
      }
    }
    return false;
  }

 private:
  char cur_[kSlots] = {0};
  char prev_[kSlots] = {0};
  uint8_t p_[kSlots] = {0};
  bool active_[kSlots] = {false};
  bool inited_[kSlots] = {false};
};
