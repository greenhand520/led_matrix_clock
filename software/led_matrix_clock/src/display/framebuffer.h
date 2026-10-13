#pragma once
// 帧缓冲与渲染（纯逻辑，无硬件依赖，可在主机侧单测）
// 坐标系：32x8 画布，x 向右 0..31，y 向下 0..7；模块 m = x/8，列 c = x%8

#include <stdint.h>

// 颜色：ORANGE = 双色同亮
enum class Color : uint8_t { OFF = 0, RED = 1, GREEN = 2, ORANGE = 3 };

// 字库（布局稿 §1）
enum class FontKind : uint8_t { BIG, SMALL, GLCD };

// 显示方向（需求 R-UI-06：正常/旋转180°/水平镜像/垂直镜像）
enum class RotateMode : uint8_t { NORMAL, ROT180, MIRROR_H, MIRROR_V };

class FrameBuffer {
 public:
  static constexpr int kWidth = 32;
  static constexpr int kHeight = 8;
  static constexpr int kModules = 4;

  void setRotate(RotateMode m) { rot_ = m; }

  void clear(Color c = Color::OFF);

  // 逻辑坐标写像素（先做方向变换再落点），越界丢弃
  void setPixel(int16_t x, int16_t y, Color c);

  // 画单字符（不推进光标）
  void drawChar(char ch, int16_t x, int16_t y, Color c, FontKind f);

  // 画字符串：字距 1 列（步距 BIG=5 / SMALL=4 / GLCD=6）
  void drawText(const char* s, int16_t x, int16_t y, Color c, FontKind f);

  // ---- 设备无关读取接口（显示后端用；换驱动芯片只改后端）----
  enum class Plane : uint8_t { RED, GREEN };

  // 模块内某行位图：bit c = 模块内列 c（bit0 = 最左列）
  uint8_t rowBits(Plane p, uint8_t module, uint8_t row) const;

  // 合并平面（红|绿）：单色后端忽略颜色时用
  uint8_t rowBitsLit(uint8_t module, uint8_t row) const;

 private:
  void plot(int16_t x, int16_t y, Color c);  // 物理坐标落点

  // [模块][行]，bit c = 模块内列 c（bit0 = 最左列）
  uint8_t red_[kModules][8];
  uint8_t green_[kModules][8];
  RotateMode rot_ = RotateMode::NORMAL;
};
