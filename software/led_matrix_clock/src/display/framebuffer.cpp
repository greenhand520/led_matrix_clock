// 帧缓冲与渲染实现（纯逻辑）
#include "display/framebuffer.h"

#include <string.h>

#include "display/font_data.h"
#include "display/glcdfont.h"

void FrameBuffer::clear(Color c) {
  const bool red = (c == Color::RED || c == Color::ORANGE);
  const bool green = (c == Color::GREEN || c == Color::ORANGE);
  memset(red_, red ? 0xFF : 0x00, sizeof(red_));
  memset(green_, green ? 0xFF : 0x00, sizeof(green_));
}

void FrameBuffer::setPixel(int16_t x, int16_t y, Color c) {
  // 方向变换（布局稿 §2）：逻辑坐标 → 物理坐标
  switch (rot_) {
    case RotateMode::NORMAL:
      break;
    case RotateMode::ROT180:
      x = (kWidth - 1) - x;
      y = (kHeight - 1) - y;
      break;
    case RotateMode::MIRROR_H:
      x = (kWidth - 1) - x;
      break;
    case RotateMode::MIRROR_V:
      y = (kHeight - 1) - y;
      break;
  }
  plot(x, y, c);
}

void FrameBuffer::plot(int16_t x, int16_t y, Color c) {
  if (x < 0 || x >= kWidth || y < 0 || y >= kHeight) {
    return;  // 越界裁剪
  }
  const uint8_t m = static_cast<uint8_t>(x / 8);
  const uint8_t bit = static_cast<uint8_t>(1 << (x % 8));
  if (c == Color::RED || c == Color::ORANGE) {
    red_[m][y] |= bit;
  }
  if (c == Color::GREEN || c == Color::ORANGE) {
    green_[m][y] |= bit;
  }
}

void FrameBuffer::drawChar(char ch, int16_t x, int16_t y, Color c, FontKind f) {
  switch (f) {
    case FontKind::BIG: {
      const uint8_t* g = bigDigit4x7(ch);
      if (g == nullptr) {
        return;
      }
      for (int r = 0; r < 7; ++r) {
        for (int col = 0; col < 4; ++col) {
          if (g[r] & (1 << col)) {
            setPixel(x + col, y + r, c);
          }
        }
      }
      break;
    }
    case FontKind::SMALL: {
      const uint8_t* g = smallGlyph3x5(ch);
      if (g == nullptr) {
        return;
      }
      for (int r = 0; r < 5; ++r) {
        for (int col = 0; col < 3; ++col) {
          if (g[r] & (1 << col)) {
            setPixel(x + col, y + r, c);
          }
        }
      }
      break;
    }
    case FontKind::GLCD: {
      // glcdfont：5 列/字符，列字节 LSB = 最上行
      const uint8_t code = static_cast<uint8_t>(ch);
      for (int col = 0; col < 5; ++col) {
        const uint8_t bits = glcdfont[code * 5 + col];
        for (int r = 0; r < 8; ++r) {
          if (bits & (1 << r)) {
            setPixel(x + col, y + r, c);
          }
        }
      }
      break;
    }
  }
}

void FrameBuffer::drawText(const char* s, int16_t x, int16_t y, Color c, FontKind f) {
  const int16_t pitch = (f == FontKind::BIG) ? 5 : (f == FontKind::SMALL) ? 4 : 6;
  for (int16_t i = 0; s[i] != '\0'; ++i) {
    drawChar(s[i], x + i * pitch, y, c, f);
  }
}

void FrameBuffer::composeToRam(uint8_t ram[kModules][16]) const {
  for (uint8_t m = 0; m < kModules; ++m) {
    for (uint8_t col = 0; col < 8; ++col) {
      uint8_t r = 0;
      uint8_t g = 0;
      for (uint8_t row = 0; row < 8; ++row) {
        const uint8_t bit = static_cast<uint8_t>(1 << col);
        if (red_[m][row] & bit) {
          r |= static_cast<uint8_t>(1 << row);
        }
        if (green_[m][row] & bit) {
          g |= static_cast<uint8_t>(1 << row);
        }
      }
      ram[m][col] = r;       // GRID col+1 = 红列
      ram[m][8 + col] = g;   // GRID col+9 = 绿列
    }
  }
}
