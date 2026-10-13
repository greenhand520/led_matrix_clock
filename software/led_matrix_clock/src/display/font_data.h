#pragma once
// 七段样式字模（由 tools/gen_font.py 生成，见布局稿 §1）
// 格式：行式，每行 1 字节，bit0 = 最左列；行序自上而下

#include <stdint.h>

// 小字中的摄氏度组合符号（°C）码值
constexpr uint8_t kCodeDegC = 0x80;

// 大数字 4x7：'0'-'9' 返回 7 行字模；其他字符返回 nullptr
const uint8_t* bigDigit4x7(char ch);

// 小字 3x5：'0'-'9'、'%'、'-'、'A'、'P'、kCodeDegC 返回 5 行字模；其他返回 nullptr
const uint8_t* smallGlyph3x5(char ch);
