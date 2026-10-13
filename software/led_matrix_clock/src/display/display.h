#pragma once
// 显示门面：绑定 DisplayDevice 后端；上电自检
// 换显示芯片时注入不同的 DisplayDevice 实现（如 MAX7219 后端），本门面与页面层不动
#include "display/display_device.h"
#include "display/framebuffer.h"

class Display {
 public:
  explicit Display(DisplayDevice& dev) : dev_(dev) {}

  // 后端初始化（含显存清零）+ 亮度设置
  void begin(uint8_t brightness);

  // 整屏刷新（预算 ≤50ms，见设计文档 §11）
  void flush(const FrameBuffer& fb);

  void setBrightness(uint8_t level);

  // 上电自检（R-UI-11）：红屏 0.25s → 绿屏 0.25s → 清屏；仅启动期调用
  void selfTest(FrameBuffer& fb);

 private:
  DisplayDevice& dev_;
};
