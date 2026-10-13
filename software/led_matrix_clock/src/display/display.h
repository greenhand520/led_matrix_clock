#pragma once
// 显示门面：帧缓冲 → TM1640 链下发；上电自检
#include "display/framebuffer.h"
#include "display/tm1640.h"

class Display {
 public:
  // 链初始化（含显存清零）+ 亮度设置
  void begin(uint8_t brightness);

  // 整屏刷新：composeToRam + 逐片写显存（预算 ≤50ms，见设计文档 §11）
  void flush(const FrameBuffer& fb);

  void setBrightness(uint8_t level);

  // 上电自检（R-UI-11）：红屏 0.25s → 绿屏 0.25s → 清屏；仅启动期调用
  void selfTest(FrameBuffer& fb);

 private:
  Tm1640Chain chain_;
};
