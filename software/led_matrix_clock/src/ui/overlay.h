#pragma once
// 状态提示覆盖层（布局稿 §5）：glcdfont 5×8、y=0..7 水平居中、ORANGE
// `Wi-Fi`/`Setup` 持续显示（sticky）；其余 2s 自动消失
// 层级：状态覆盖 > 日期滚动 > 页面内容
#include <stdint.h>
#include <string.h>

#include "display/framebuffer.h"

class StatusOverlay {
 public:
  // sticky=true 持续显示（Wi-Fi/Setup）；否则 2s 后消失
  void show(const char* text, uint32_t nowMs, bool sticky) {
    strncpy(text_, text, sizeof(text_) - 1);
    text_[sizeof(text_) - 1] = '\0';
    active_ = true;
    sticky_ = sticky;
    untilMs_ = nowMs + 2000;
  }

  void clear() { active_ = false; }

  bool active(uint32_t nowMs) const {
    return active_ && (sticky_ || static_cast<int32_t>(nowMs - untilMs_) < 0);
  }

  void draw(FrameBuffer& fb) const {
    if (!active_) {
      return;
    }
    // 居中：x = (32 - (6n-1)) / 2（glcdfont 步距 6）
    const int16_t w = static_cast<int16_t>(strlen(text_) * 6 - 1);
    const int16_t x = static_cast<int16_t>((FrameBuffer::kWidth - w) / 2);
    fb.drawText(text_, x, 0, Color::ORANGE, FontKind::GLCD);
  }

 private:
  char text_[16] = {0};
  uint32_t untilMs_ = 0;
  bool active_ = false;
  bool sticky_ = false;
};
