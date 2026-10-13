#pragma once
// 文本水平滚动（纯逻辑，可主机侧单测；布局稿 §4）：
// 文本从右边缘进入、左边缘移出，50ms/步；glcdfont 步距 6 列
#include <stdint.h>
#include <string.h>

#include "display/framebuffer.h"

class TextScroller {
 public:
  static constexpr int kPitch = 6;  // glcdfont 5+1

  void start(const char* text) {
    strncpy(text_, text, sizeof(text_) - 1);
    text_[sizeof(text_) - 1] = '\0';
    x_ = FrameBuffer::kWidth;  // 屏右外
    running_ = text_[0] != '\0';
  }

  void step() {
    if (!running_) {
      return;
    }
    --x_;
    const int16_t width = static_cast<int16_t>(strlen(text_) * kPitch);
    if (static_cast<int16_t>(x_ + width) <= 0) {
      running_ = false;  // 尾部离开左边缘
    }
  }

  bool running() const { return running_; }
  int16_t x() const { return x_; }

  void draw(FrameBuffer& fb, int16_t y, Color c, FontKind f) const {
    if (running_) {
      fb.drawText(text_, x_, y, c, f);
    }
  }

 private:
  char text_[32] = {0};
  int16_t x_ = 0;
  bool running_ = false;
};
