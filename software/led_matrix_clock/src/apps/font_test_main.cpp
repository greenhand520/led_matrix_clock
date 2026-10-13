// 显示字模/像素验收固件入口（PlatformIO 环境 esp12e_fonttest，非产品固件）
// 用途：硬件上逐屏检视全部字形、双色合成、方向与亮度（P2 验收工具）
// 自动循环 6 个阶段（每步 1s）：
//   1) glcdfont 0x20-0x7F：每屏 4 字符 + 右侧 2 位序号（屏索引 00..23）
//   2) 大数字 4x7 的 0-9：每屏 4 位
//   3) 小字 3x5 全符号（数字/％/-/A/P/°C）：每屏 6 个
//   4) 颜色：红/绿/橙全屏 + 双色棋盘
//   5) 方向：同一不对称图案按 正常/旋转180°/水平镜像/垂直镜像
//   6) 亮度阶梯 0..7（橙色全屏）
#include <Arduino.h>

#include "display/display.h"
#include "display/font_data.h"

namespace {

Display g_display;
FrameBuffer g_fb;

constexpr uint32_t kStepMs = 1000;
constexpr uint8_t kDefaultBrightness = 5;

enum Phase : uint8_t {
  PHASE_GLCD = 0,
  PHASE_BIG,
  PHASE_SMALL,
  PHASE_COLOR,
  PHASE_ROTATE,
  PHASE_BRIGHT,
  PHASE_COUNT,
};

uint8_t g_phase = 0;
uint8_t g_step = 0;
uint32_t g_nextMs = 0;

uint8_t stepsOf(uint8_t phase) {
  switch (phase) {
    case PHASE_GLCD:
      return 24;  // 96 字符 / 4 每屏
    case PHASE_BIG:
      return 3;  // 10 位 / 4 每屏
    case PHASE_SMALL:
      return 3;  // 15 符号 / 6 每屏
    case PHASE_COLOR:
      return 4;
    case PHASE_ROTATE:
      return 4;  // RotateMode 四模式
    default:
      return 8;  // 亮度 0..7
  }
}

// 右侧 2 位序号（小字绿）
void drawIndex(uint8_t v) {
  g_fb.drawChar(char('0' + v / 10), 24, 2, Color::GREEN, FontKind::SMALL);
  g_fb.drawChar(char('0' + v % 10), 28, 2, Color::GREEN, FontKind::SMALL);
}

void renderStep() {
  g_fb.clear();
  g_fb.setRotate(RotateMode::NORMAL);
  switch (g_phase) {
    case PHASE_GLCD: {
      const uint8_t code = static_cast<uint8_t>(0x20 + g_step * 4);
      for (uint8_t i = 0; i < 4; ++i) {
        if (code + i <= 0x7F) {
          g_fb.drawChar(static_cast<char>(code + i), i * 6, 0, Color::RED, FontKind::GLCD);
        }
      }
      drawIndex(g_step);
      break;
    }
    case PHASE_BIG: {
      const uint8_t first = static_cast<uint8_t>(g_step * 4);
      for (uint8_t i = 0; i < 4 && first + i <= 9; ++i) {
        g_fb.drawChar(char('0' + first + i), i * 5, 0, Color::RED, FontKind::BIG);
      }
      drawIndex(first);
      break;
    }
    case PHASE_SMALL: {
      static const char kAll[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                                  '%', '-', 'A', 'P', static_cast<char>(kCodeDegC)};
      const uint8_t first = static_cast<uint8_t>(g_step * 6);
      for (uint8_t i = 0; i < 6 && first + i < 15; ++i) {
        g_fb.drawChar(kAll[first + i], i * 4, 2, Color::GREEN, FontKind::SMALL);
      }
      drawIndex(first);
      break;
    }
    case PHASE_COLOR:
      if (g_step == 0) {
        g_fb.clear(Color::RED);
      } else if (g_step == 1) {
        g_fb.clear(Color::GREEN);
      } else if (g_step == 2) {
        g_fb.clear(Color::ORANGE);
      } else {
        for (int16_t y = 0; y < FrameBuffer::kHeight; ++y) {
          for (int16_t x = 0; x < FrameBuffer::kWidth; ++x) {
            g_fb.setPixel(x, y, ((x + y) & 1) ? Color::RED : Color::GREEN);
          }
        }
      }
      break;
    case PHASE_ROTATE: {
      // 不对称图案，便于分辨方向
      g_fb.setRotate(static_cast<RotateMode>(g_step));
      g_fb.drawChar('1', 2, 0, Color::RED, FontKind::BIG);
      g_fb.drawChar('2', 8, 0, Color::RED, FontKind::BIG);
      g_fb.drawChar('3', 16, 2, Color::GREEN, FontKind::SMALL);
      g_fb.drawChar('4', 20, 2, Color::GREEN, FontKind::SMALL);
      for (uint8_t r = 0; r < 2; ++r) {
        for (uint8_t c = 0; c < 3; ++c) {
          g_fb.setPixel(28 + c, r, Color::ORANGE);  // 右上角色块
        }
      }
      break;
    }
    case PHASE_BRIGHT:
      g_display.setBrightness(g_step);
      g_fb.clear(Color::ORANGE);
      break;
    default:
      break;
  }
  g_display.flush(g_fb);
}

void advance() {
  ++g_step;
  if (g_step >= stepsOf(g_phase)) {
    g_step = 0;
    g_phase = static_cast<uint8_t>((g_phase + 1) % PHASE_COUNT);
    if (g_phase != PHASE_BRIGHT) {
      g_display.setBrightness(kDefaultBrightness);  // 离开亮度阶段时恢复
    }
    Serial.printf("[fonttest] phase %u\n", g_phase);
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println(F("[boot] display font/pixel test firmware"));
  g_display.begin(kDefaultBrightness);
  g_display.selfTest(g_fb);
  renderStep();
}

void loop() {
  const uint32_t now = millis();
  if (static_cast<int32_t>(now - g_nextMs) >= 0) {
    g_nextMs = now + kStepMs;
    advance();
    renderStep();
  }
  yield();
}
