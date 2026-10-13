#pragma once
// 显示后端接口：更换驱动芯片（如 MAX7219）时实现本接口即可，渲染层/页面层不动。
// 模块拓扑恒为 8x8 点阵级联（kModules 个）；单色后端忽略颜色平面（红/绿任一亮即点亮，
// 可用 FrameBuffer::rowBitsLit 合并平面）。
#include <stdint.h>

#include "display/framebuffer.h"

class DisplayDevice {
 public:
  virtual ~DisplayDevice() = default;

  // 初始化（含芯片级清显存等）+ 亮度
  virtual void begin(uint8_t brightness) = 0;

  // 整屏下发
  virtual void writeFrame(const FrameBuffer& fb) = 0;

  virtual void setBrightness(uint8_t level) = 0;
  virtual void setDisplayOn(bool on) = 0;
};
